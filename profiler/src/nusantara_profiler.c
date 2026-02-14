#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#define MODULE_CONFIG "/data/adb/.config/Nusantara"
#define MAX_PATH_LEN 256
#define MAX_LINE_LEN 1024
#define MAX_OPP_COUNT 50

// Global variables
int SOC = 0;
int LITE_MODE = 0;
int DEVICE_MITIGATION = 0;
char DEFAULT_CPU_GOV[50] = "schedutil";
char PPM_POLICY[512] = "";

// Function prototypes
void read_configs();
int apply(const char *value, const char *path);
int write_file(const char *value, const char *path);
int apply_ll(long long value, const char *path);
int write_ll(long long value, const char *path);
void change_cpu_gov(const char *gov);
void set_dnd(int mode);
long get_max_freq(const char *path);
long get_min_freq(const char *path);
long get_mid_freq(const char *path);
int mtk_gpufreq_minfreq_index(const char *path);
int mtk_gpufreq_midfreq_index(const char *path);
void cpufreq_ppm_max_perf();
void cpufreq_max_perf();
void cpufreq_ppm_unlock();
void cpufreq_unlock();
int devfreq_max_perf(const char *path);
int devfreq_mid_perf(const char *path);
int devfreq_unlock(const char *path);
int devfreq_min_perf(const char *path);
int qcom_cpudcvs_max_perf(const char *path);
int qcom_cpudcvs_mid_perf(const char *path);
int qcom_cpudcvs_unlock(const char *path);
int qcom_cpudcvs_min_perf(const char *path);
void mediatek_performance();
void snapdragon_performance();
void exynos_performance();
void unisoc_performance();
void tensor_performance();
void mediatek_normal();
void snapdragon_normal();
void exynos_normal();
void unisoc_normal();
void tensor_normal();
void mediatek_powersave();
void snapdragon_powersave();
void exynos_powersave();
void unisoc_powersave();
void tensor_powersave();
void perfcommon();
void performance_profile();
void normal_profile();
void powersave_profile();

// Utility functions
int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

int read_int_from_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    int value;
    fscanf(fp, "%d", &value);
    fclose(fp);
    return value;
}

void read_string_from_file(char *buffer, size_t size, const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        buffer[0] = '\0';
        return;
    }
    fgets(buffer, size, fp);
    buffer[strcspn(buffer, "\n")] = '\0';
    fclose(fp);
}

int apply(const char *value, const char *path) {
    if (!file_exists(path)) return 0;
    chmod(path, 0644);
    FILE *fp = fopen(path, "w");
    if (!fp) {
        chmod(path, 0444);
        return 0;
    }
    fprintf(fp, "%s", value);
    fclose(fp);
    chmod(path, 0444);
    return 1;
}

int write_file(const char *value, const char *path) {
    if (!file_exists(path)) return 0;    
    chmod(path, 0644);
    FILE *fp = fopen(path, "w");
    if (!fp) {
        chmod(path, 0444);
        return 0;
    }
    fprintf(fp, "%s", value);
    fclose(fp);
    chmod(path, 0444);
    return 1;
}

void change_cpu_gov(const char *gov) {
    DIR *dir;
    struct dirent *ent;
    char path[MAX_PATH_LEN];
    // Change governor for each CPU
    for (int i = 0; i <= 15; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", i);
        if (file_exists(path)) {
            chmod(path, 0644);
            write_file(gov, path);
        }
    }    
    // Also update policy governors
    dir = opendir("/sys/devices/system/cpu/cpufreq");
    if (dir) {
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, "policy")) {
                snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpufreq/%s/scaling_governor", ent->d_name);
                if (file_exists(path)) {
                    chmod(path, 0444);
                }
            }
        }
        closedir(dir);
    }
}

void set_dnd(int mode) {
    if (mode == 0) {
        system("cmd notification set_dnd off");
    } else if (mode == 1) {
        system("cmd notification set_dnd priority");
    }
}

// Frequency calculation functions
long get_max_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    long max_freq = 0;
    long freq;
    while (fscanf(fp, "%ld", &freq) == 1) {
        if (freq > max_freq) max_freq = freq;
    }
    fclose(fp);
    return max_freq;
}

long get_min_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    long min_freq = LONG_MAX;
    long freq;
    while (fscanf(fp, "%ld", &freq) == 1) {
        if (freq > 0 && freq < min_freq) min_freq = freq;
    }
    fclose(fp);
    return (min_freq == LONG_MAX) ? 0 : min_freq;
}

long get_mid_freq(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    long freqs[MAX_OPP_COUNT];
    int count = 0;
    while (fscanf(fp, "%ld", &freqs[count]) == 1 && count < MAX_OPP_COUNT - 1) {
        count++;
    }
    fclose(fp);
    if (count == 0) return 0;
    // Sort frequencies
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (freqs[j] > freqs[j + 1]) {
                long temp = freqs[j];
                freqs[j] = freqs[j + 1];
                freqs[j + 1] = temp;
            }
        }
    }
    // Get middle frequency
    return freqs[count / 2];
}

int mtk_gpufreq_minfreq_index(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char line[MAX_LINE_LEN];
    int min_index = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "freq =")) {
            // Parse index from format like "[index] freq = xxx"
            char *start = strchr(line, '[');
            if (start) {
                char *end = strchr(start, ']');
                if (end) {
                    *end = '\0';
                    int index = atoi(start + 1);
                    min_index = index;
                }
            }
        }
    } 
    fclose(fp);
    return min_index;
}

int mtk_gpufreq_midfreq_index(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char line[MAX_LINE_LEN];
    int indices[MAX_OPP_COUNT];
    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < MAX_OPP_COUNT) {
        if (strstr(line, "freq =")) {
            char *start = strchr(line, '[');
            if (start) {
                char *end = strchr(start, ']');
                if (end) {
                    *end = '\0';
                    indices[count++] = atoi(start + 1);
                }
            }
        }
    }
    fclose(fp);
    if (count == 0) return 0;
    return indices[count / 2];
}

// CPU frequency settings
void cpufreq_ppm_max_perf() {
    DIR *dir;
    struct dirent *ent;
    char path[MAX_PATH_LEN];
    int cluster = -1;
    dir = opendir("/sys/devices/system/cpu/cpufreq");
    if (!dir) return;
    while ((ent = readdir(dir)) != NULL) {
        if (strstr(ent->d_name, "policy")) {
            cluster++;
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpufreq/%s/cpuinfo_max_freq", ent->d_name);
            long cpu_maxfreq = read_int_from_file(path);
            char ppm_cmd[100];
            snprintf(ppm_cmd, sizeof(ppm_cmd), "%d %ld", cluster, cpu_maxfreq);
            apply(ppm_cmd, "/proc/ppm/policy/hard_userlimit_max_cpu_freq");
            if (LITE_MODE == 1) {
                snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpufreq/%s/scaling_available_frequencies", ent->d_name);
                long cpu_midfreq = get_mid_freq(path);
                snprintf(ppm_cmd, sizeof(ppm_cmd), "%d %ld", cluster, cpu_midfreq);
                apply(ppm_cmd, "/proc/ppm/policy/hard_userlimit_min_cpu_freq");
            } else {
                snprintf(ppm_cmd, sizeof(ppm_cmd), "%d %ld", cluster, cpu_maxfreq);
                apply(ppm_cmd, "/proc/ppm/policy/hard_userlimit_min_cpu_freq");
            }
        }
    }
    closedir(dir);
}

void cpufreq_max_perf() {
    for (int i = 0; i <= 15; i++) {
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
        if (!file_exists(path)) continue;
        long cpu_maxfreq = read_int_from_file(path);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        apply_ll(cpu_maxfreq, path);
        if (LITE_MODE == 1) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_available_frequencies", i);
            long cpu_midfreq = get_mid_freq(path);
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            apply_ll(cpu_midfreq, path);
        } else {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            apply_ll(cpu_maxfreq, path);
        }
    }
    // Set permissions
    DIR *dir = opendir("/sys/devices/system/cpu/cpufreq");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, "policy")) {
                char policy_path[MAX_PATH_LEN];
                snprintf(policy_path, sizeof(policy_path), "/sys/devices/system/cpu/cpufreq/%s/scaling_max_freq", ent->d_name);
                chmod(policy_path, 0444);
                
                snprintf(policy_path, sizeof(policy_path), "/sys/devices/system/cpu/cpufreq/%s/scaling_min_freq", ent->d_name);
                chmod(policy_path, 0444);
            }
        }
        closedir(dir);
    }
}

void cpufreq_ppm_unlock() {
    DIR *dir;
    struct dirent *ent;
    char path[MAX_PATH_LEN];
    int cluster = 0; 
    dir = opendir("/sys/devices/system/cpu/cpufreq");
    if (!dir) return;
    while ((ent = readdir(dir)) != NULL) {
        if (strstr(ent->d_name, "policy")) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpufreq/%s/cpuinfo_max_freq", ent->d_name);
            long cpu_maxfreq = read_int_from_file(path);
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpufreq/%s/cpuinfo_min_freq", ent->d_name);
            long cpu_minfreq = read_int_from_file(path);
            char ppm_cmd[100];
            snprintf(ppm_cmd, sizeof(ppm_cmd), "%d %ld", cluster, cpu_maxfreq);
            write_file(ppm_cmd, "/proc/ppm/policy/hard_userlimit_max_cpu_freq");
            snprintf(ppm_cmd, sizeof(ppm_cmd), "%d %ld", cluster, cpu_minfreq);
            write_file(ppm_cmd, "/proc/ppm/policy/hard_userlimit_min_cpu_freq");
            
            cluster++;
        }
    }
    closedir(dir);
}

void cpufreq_unlock() {
    for (int i = 0; i <= 15; i++) {
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
        if (!file_exists(path)) continue;
        long cpu_maxfreq = read_int_from_file(path);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_min_freq", i);
        long cpu_minfreq = read_int_from_file(path);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        write_ll(cpu_maxfreq, path);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        write_ll(cpu_minfreq, path);
    }
    // Set permissions
    DIR *dir = opendir("/sys/devices/system/cpu/cpufreq");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, "policy")) {
                char policy_path[MAX_PATH_LEN];
                snprintf(policy_path, sizeof(policy_path), "/sys/devices/system/cpu/cpufreq/%s/scaling_max_freq", ent->d_name);
                chmod(policy_path, 0644);
                
                snprintf(policy_path, sizeof(policy_path), "/sys/devices/system/cpu/cpufreq/%s/scaling_min_freq", ent->d_name);
                chmod(policy_path, 0644);
            }
        }
        closedir(dir);
    }
}

int devfreq_max_perf(const char *path) {
    if (!file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/max_freq", path);
    apply_ll(max_freq, max_path);
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/min_freq", path);
    apply_ll(max_freq, min_path);
    
    return 1;
}

int devfreq_mid_perf(const char *path) {
    if (!file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long mid_freq = get_mid_freq(freq_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/max_freq", path);
    apply_ll(max_freq, max_path); 
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/min_freq", path);
    apply_ll(mid_freq, min_path);
    return 1;
}

int devfreq_unlock(const char *path) {
    if (!file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long min_freq = get_min_freq(freq_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/max_freq", path);
    write_ll(max_freq, max_path);
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/min_freq", path);
    write_ll(min_freq, min_path);
    return 1;
}

int devfreq_min_perf(const char *path) {
    if (!file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long freq = get_min_freq(freq_path);
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/min_freq", path);
    apply_ll(freq, min_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/max_freq", path);
    apply_ll(freq, max_path);
    return 1;
}

int qcom_cpudcvs_max_perf(const char *path) {
    if (!file_exists(path)) return 0; 
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long freq = get_max_freq(freq_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/hw_max_freq", path);
    apply_ll(freq, max_path);
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/hw_min_freq", path);
    apply_ll(freq, min_path);
    return 1;
}

int qcom_cpudcvs_mid_perf(const char *path) {
    if (!file_exists(path)) return 0; 
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long mid_freq = get_mid_freq(freq_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/hw_max_freq", path);
    apply_ll(max_freq, max_path);
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/hw_min_freq", path);
    apply_ll(mid_freq, min_path);
    
    return 1;
}

int qcom_cpudcvs_unlock(const char *path) {
    if (!file_exists(path)) return 0;
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long max_freq = get_max_freq(freq_path);
    long min_freq = get_min_freq(freq_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/hw_max_freq", path);
    write_ll(max_freq, max_path);
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/hw_min_freq", path);
    write_ll(min_freq, min_path);
    return 1;
}

int qcom_cpudcvs_min_perf(const char *path) {
    if (!file_exists(path)) return 0; 
    char freq_path[MAX_PATH_LEN];
    snprintf(freq_path, sizeof(freq_path), "%s/available_frequencies", path);
    if (!file_exists(freq_path)) return 0;
    long freq = get_min_freq(freq_path);
    char min_path[MAX_PATH_LEN];
    snprintf(min_path, sizeof(min_path), "%s/hw_min_freq", path);
    apply_ll(freq, min_path);
    char max_path[MAX_PATH_LEN];
    snprintf(max_path, sizeof(max_path), "%s/hw_max_freq", path);
    apply_ll(freq, max_path);
    return 1;
}

// Helper function for long long values
int apply_ll(long long value, const char *path) {
    char str[50];
    snprintf(str, sizeof(str), "%lld", value);
    return apply(str, path);
}

int write_ll(long long value, const char *path) {
    char str[50];
    snprintf(str, sizeof(str), "%lld", value);
    return write_file(str, path);
}

// Device-specific performance functions
void mediatek_performance() {
    // PPM policies
    if (file_exists("/proc/ppm/policy_status")) {
        FILE *fp = fopen("/proc/ppm/policy_status", "r");
        if (fp) {
            char line[MAX_LINE_LEN];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, PPM_POLICY)) {
                    char policy_cmd[10];
                    snprintf(policy_cmd, sizeof(policy_cmd), "%c 0", line[1]);
                    apply(policy_cmd, "/proc/ppm/policy_status");
                }
            }
            fclose(fp);
        }
    }
    
    // Force off FPSGO
    apply("0", "/sys/kernel/fpsgo/common/force_onoff");
    apply("1", "/sys/pnpmgr/fpsgo_boost/boost_enable");
    apply("1", "/sys/module/mtk_fpsgo/parameters/perfmgr_enable");    
     
    // MTK Power and CCI mode
    apply("1", "/proc/cpufreq/cpufreq_cci_mode");
    apply("3", "/proc/cpufreq/cpufreq_power_mode");
    
     // Perf limiter ON
    apply("1", "/proc/perfmgr/syslimiter/syslimiter_force_disable");
    
    // DDR Boost mode
    apply("1", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");
    
    // EAS/HMP Switch
    apply("0", "/sys/devices/system/cpu/eas/enable");
    
    // Disable GED KPI
    apply("0", "/sys/module/sspm_v3/holders/ged/parameters/is_GED_KPI_enabled");
    
    // GPU Frequency
    if (LITE_MODE == 0) {
        if (file_exists("/proc/gpufreqv2")) {
            apply("0", "/proc/gpufreqv2/fix_target_opp_index");
        } else if (file_exists("/proc/gpufreq/gpufreq_opp_dump")) {
            FILE *fp = fopen("/proc/gpufreq/gpufreq_opp_dump", "r");
            if (fp) {
                char line[MAX_LINE_LEN];
                if (fgets(line, sizeof(line), fp)) {
                    char *freq_str = strstr(line, "freq =");
                    if (freq_str) {
                        long freq = atol(freq_str + 6);
                        apply_ll(freq, "/proc/gpufreq/gpufreq_opp_freq");
                    }
                }
                fclose(fp);
            }
        }
    } else {
        apply("0", "/proc/gpufreq/gpufreq_opp_freq");
        apply("-1", "/proc/gpufreqv2/fix_target_opp_index");
        
        // Set min freq via GED
        int mid_oppfreq;
        if (file_exists("/proc/gpufreqv2/gpu_working_opp_table")) {
            mid_oppfreq = mtk_gpufreq_midfreq_index("/proc/gpufreqv2/gpu_working_opp_table");
        } else if (file_exists("/proc/gpufreq/gpufreq_opp_dump")) {
            mid_oppfreq = mtk_gpufreq_midfreq_index("/proc/gpufreq/gpufreq_opp_dump");
        } else {
            mid_oppfreq = 0;
        }
        
        apply_ll(mid_oppfreq, "/sys/kernel/ged/hal/custom_boost_gpu_freq");
    }
    
    // Disable GPU Power limiter
    if (file_exists("/proc/gpufreq/gpufreq_power_limited")) {
        apply("ignore_batt_oc 1", "/proc/gpufreq/gpufreq_power_limited");
        apply("ignore_batt_percent 1", "/proc/gpufreq/gpufreq_power_limited");
        apply("ignore_low_batt 1", "/proc/gpufreq/gpufreq_power_limited");
        apply("ignore_thermal_protect 1", "/proc/gpufreq/gpufreq_power_limited");
        apply("ignore_pbm_limited 1", "/proc/gpufreq/gpufreq_power_limited");
    }
    
    // Disable battery current limiter
    apply("stop 1", "/proc/mtk_batoc_throttling/battery_oc_protect_stop");
    
    // DRAM Frequency
    if (LITE_MODE == 0) {
        apply("0", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
        apply("0", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
        devfreq_max_perf("/sys/class/devfreq/mtk-dvfsrc-devfreq");
    } else {
        apply("-1", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
        apply("-1", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
        devfreq_mid_perf("/sys/class/devfreq/mtk-dvfsrc-devfreq");
    }
    
    // Eara Thermal
    apply("0", "/sys/kernel/eara_thermal/enable");
}

void snapdragon_performance() {
    // Qualcomm CPU Bus and DRAM frequencies
    if (DEVICE_MITIGATION == 0) {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                char *name = ent->d_name;
                if (strstr(name, "cpu-lat") || strstr(name, "cpu-bw") || 
                    strstr(name, "llccbw") || strstr(name, "bus_llcc") ||
                    strstr(name, "bus_ddr") || strstr(name, "memlat") ||
                    strstr(name, "cpubw") || strstr(name, "kgsl-ddr-qos")) {
                    char path[MAX_PATH_LEN];
                    snprintf(path, sizeof(path), "/sys/class/devfreq/%s", name);
                    if (LITE_MODE == 1) {
                        devfreq_mid_perf(path);
                    } else {
                        devfreq_max_perf(path);
                    }
                }
            }
            closedir(dir);
        }
        
        // Process bus_dcvs components
        const char *components[] = {"DDR", "LLCC", "L3"};
        for (int i = 0; i < 3; i++) {
            char path[MAX_PATH_LEN];
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/bus_dcvs/%s", components[i]);
            
            if (LITE_MODE == 1) {
                qcom_cpudcvs_mid_perf(path);
            } else {
                qcom_cpudcvs_max_perf(path);
            }
        }
    }
    
    // GPU tweak
    const char *gpu_path = "/sys/class/kgsl/kgsl-3d0/devfreq";
    if (LITE_MODE == 0) {
        devfreq_max_perf(gpu_path);
    } else {
        devfreq_mid_perf(gpu_path);
    }
    
    // Disable GPU Bus split
    apply("0", "/sys/class/kgsl/kgsl-3d0/bus_split");
    
    // Force GPU clock on
    apply("1", "/sys/class/kgsl/kgsl-3d0/force_clk_on");
}

void exynos_performance() {
    // GPU Frequency
    const char *gpu_path = "/sys/kernel/gpu";
    if (file_exists(gpu_path)) {
        char avail_path[MAX_PATH_LEN];
        snprintf(avail_path, sizeof(avail_path), "%s/gpu_available_frequencies", gpu_path);
        
        long max_freq = get_max_freq(avail_path);
        char max_clock_path[MAX_PATH_LEN];
        snprintf(max_clock_path, sizeof(max_clock_path), "%s/gpu_max_clock", gpu_path);
        apply_ll(max_freq, max_clock_path);
        
        if (LITE_MODE == 1) {
            long mid_freq = get_mid_freq(avail_path);
            char min_clock_path[MAX_PATH_LEN];
            snprintf(min_clock_path, sizeof(min_clock_path), "%s/gpu_min_clock", gpu_path);
            apply_ll(mid_freq, min_clock_path);
        } else {
            char min_clock_path[MAX_PATH_LEN];
            snprintf(min_clock_path, sizeof(min_clock_path), "%s/gpu_min_clock", gpu_path);
            apply_ll(max_freq, min_clock_path);
        }
    }
    
    // Find mali sysfs
    DIR *dir = opendir("/sys/devices/platform");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".mali")) {
                char mali_path[MAX_PATH_LEN];
                snprintf(mali_path, sizeof(mali_path), "/sys/devices/platform/%s/power_policy", ent->d_name);
                apply("always_on", mali_path);
                break;
            }
        }
        closedir(dir);
    }
    
    // DRAM and Buses Frequency
    if (DEVICE_MITIGATION == 0) {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (strstr(ent->d_name, "devfreq_mif")) {
                    char path[MAX_PATH_LEN];
                    snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
                    
                    if (LITE_MODE == 1) {
                        devfreq_mid_perf(path);
                    } else {
                        devfreq_max_perf(path);
                    }
                }
            }
            closedir(dir);
        }
    }
}

void unisoc_performance() {
    // Find GPU path
    DIR *dir = opendir("/sys/class/devfreq");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".gpu")) {
                char gpu_path[MAX_PATH_LEN];
                snprintf(gpu_path, sizeof(gpu_path), "/sys/class/devfreq/%s", ent->d_name);
                
                if (LITE_MODE == 0) {
                    devfreq_max_perf(gpu_path);
                } else {
                    devfreq_mid_perf(gpu_path);
                }
                break;
            }
        }
        closedir(dir);
    }
}

void tensor_performance() {
    // Find GPU path
    DIR *dir = opendir("/sys/devices/platform");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".mali")) {
                char gpu_path[MAX_PATH_LEN];
                snprintf(gpu_path, sizeof(gpu_path), "/sys/devices/platform/%s", ent->d_name);
                
                char avail_path[MAX_PATH_LEN];
                snprintf(avail_path, sizeof(avail_path), "%s/available_frequencies", gpu_path);
                
                if (file_exists(avail_path)) {
                    long max_freq = get_max_freq(avail_path);
                    
                    char max_freq_path[MAX_PATH_LEN];
                    snprintf(max_freq_path, sizeof(max_freq_path), "%s/scaling_max_freq", gpu_path);
                    apply_ll(max_freq, max_freq_path);
                    
                    if (LITE_MODE == 1) {
                        long mid_freq = get_mid_freq(avail_path);
                        char min_freq_path[MAX_PATH_LEN];
                        snprintf(min_freq_path, sizeof(min_freq_path), "%s/scaling_min_freq", gpu_path);
                        apply_ll(mid_freq, min_freq_path);
                    } else {
                        char min_freq_path[MAX_PATH_LEN];
                        snprintf(min_freq_path, sizeof(min_freq_path), "%s/scaling_min_freq", gpu_path);
                        apply_ll(max_freq, min_freq_path);
                    }
                }
                break;
            }
        }
        closedir(dir);
    }
    
    // DRAM frequency
    if (DEVICE_MITIGATION == 0) {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (strstr(ent->d_name, "devfreq_mif")) {
                    char path[MAX_PATH_LEN];
                    snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
                    
                    if (LITE_MODE == 1) {
                        devfreq_mid_perf(path);
                    } else {
                        devfreq_max_perf(path);
                    }
                }
            }
            closedir(dir);
        }
    }
}

// Device-specific normal profile functions
void mediatek_normal() {
    // PPM policies
    if (file_exists("/proc/ppm/policy_status")) {
        FILE *fp = fopen("/proc/ppm/policy_status", "r");
        if (fp) {
            char line[MAX_LINE_LEN];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, PPM_POLICY)) {
                    char policy_cmd[10];
                    snprintf(policy_cmd, sizeof(policy_cmd), "%c 1", line[1]);
                    apply(policy_cmd, "/proc/ppm/policy_status");
                }
            }
            fclose(fp);
        }
    }
    
    // Free FPSGO
    apply("2", "/sys/kernel/fpsgo/common/force_onoff");
    apply("1", "/sys/pnpmgr/fpsgo_boost/boost_enable");
    apply("1", "/sys/module/mtk_fpsgo/parameters/perfmgr_enable");
    
    // MTK Power and CCI mode
    apply("0", "/proc/cpufreq/cpufreq_cci_mode");
    apply("0", "/proc/cpufreq/cpufreq_power_mode");
    
    // Perf limiter ON
    apply("0", "/proc/perfmgr/syslimiter/syslimiter_force_disable");
    
    // DDR Boost mode
    apply("0", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");
    
    // EAS/HMP Switch
    apply("2", "/sys/devices/system/cpu/eas/enable");
    
    // Enable GED KPI
    apply("1", "/sys/module/sspm_v3/holders/ged/parameters/is_GED_KPI_enabled");
    
    // GPU Frequency
    write_file("0", "/proc/gpufreq/gpufreq_opp_freq");
    write_file("-1", "/proc/gpufreqv2/fix_target_opp_index");
    
    // Reset min freq via GED
    int min_oppfreq;
    if (file_exists("/proc/gpufreqv2/gpu_working_opp_table")) {
        min_oppfreq = mtk_gpufreq_minfreq_index("/proc/gpufreqv2/gpu_working_opp_table");
    } else if (file_exists("/proc/gpufreq/gpufreq_opp_dump")) {
        min_oppfreq = mtk_gpufreq_minfreq_index("/proc/gpufreq/gpufreq_opp_dump");
    } else {
        min_oppfreq = 0;
    }
    
    apply_ll(min_oppfreq, "/sys/kernel/ged/hal/custom_boost_gpu_freq");
    
    // GPU Power limiter
    if (file_exists("/proc/gpufreq/gpufreq_power_limited")) {
        apply("ignore_batt_oc 0", "/proc/gpufreq/gpufreq_power_limited");
        apply("ignore_batt_percent 0", "/proc/gpufreq/gpufreq_power_limited");
        apply("ignore_low_batt 0", "/proc/gpufreq/gpufreq_power_limited");
        apply("ignore_thermal_protect 0", "/proc/gpufreq/gpufreq_power_limited");
        apply("ignore_pbm_limited 0", "/proc/gpufreq/gpufreq_power_limited");
    }
    
    // Enable battery current limiter
    apply("stop 0", "/proc/mtk_batoc_throttling/battery_oc_protect_stop");
    
    // DRAM Frequency
    write_file("-1", "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp");
    write_file("-1", "/sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp");
    devfreq_unlock("/sys/class/devfreq/mtk-dvfsrc-devfreq");
    
    // Eara Thermal
    apply("1", "/sys/kernel/eara_thermal/enable");
}

void snapdragon_normal() {
    // Qualcomm CPU Bus and DRAM frequencies
    if (DEVICE_MITIGATION == 0) {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                char *name = ent->d_name;
                if (strstr(name, "cpu-lat") || strstr(name, "cpu-bw") || 
                    strstr(name, "llccbw") || strstr(name, "bus_llcc") ||
                    strstr(name, "bus_ddr") || strstr(name, "memlat") ||
                    strstr(name, "cpubw") || strstr(name, "kgsl-ddr-qos")) {
                    
                    char path[MAX_PATH_LEN];
                    snprintf(path, sizeof(path), "/sys/class/devfreq/%s", name);
                    devfreq_unlock(path);
                }
            }
            closedir(dir);
        }
        
        // Process bus_dcvs components
        const char *components[] = {"DDR", "LLCC", "L3"};
        for (int i = 0; i < 3; i++) {
            char path[MAX_PATH_LEN];
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/bus_dcvs/%s", components[i]);
            qcom_cpudcvs_unlock(path);
        }
    }
    
    // Revert GPU tweak
    devfreq_unlock("/sys/class/kgsl/kgsl-3d0/devfreq");
    
    // Enable back GPU Bus split
    apply("1", "/sys/class/kgsl/kgsl-3d0/bus_split");
    
    // Free GPU clock on/off
    apply("0", "/sys/class/kgsl/kgsl-3d0/force_clk_on");
}

void exynos_normal() {
    // GPU Frequency
    const char *gpu_path = "/sys/kernel/gpu";
    if (file_exists(gpu_path)) {
        char avail_path[MAX_PATH_LEN];
        snprintf(avail_path, sizeof(avail_path), "%s/gpu_available_frequencies", gpu_path);
        
        long max_freq = get_max_freq(avail_path);
        long min_freq = get_min_freq(avail_path);
        
        char max_clock_path[MAX_PATH_LEN];
        snprintf(max_clock_path, sizeof(max_clock_path), "%s/gpu_max_clock", gpu_path);
        write_ll(max_freq, max_clock_path);
        
        char min_clock_path[MAX_PATH_LEN];
        snprintf(min_clock_path, sizeof(min_clock_path), "%s/gpu_min_clock", gpu_path);
        write_ll(min_freq, min_clock_path);
    }
    
    // Find mali sysfs
    DIR *dir = opendir("/sys/devices/platform");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".mali")) {
                char mali_path[MAX_PATH_LEN];
                snprintf(mali_path, sizeof(mali_path), "/sys/devices/platform/%s/power_policy", ent->d_name);
                apply("coarse_demand", mali_path);
                break;
            }
        }
        closedir(dir);
    }
    
    // DRAM frequency
    if (DEVICE_MITIGATION == 0) {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (strstr(ent->d_name, "devfreq_mif")) {
                    char path[MAX_PATH_LEN];
                    snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
                    devfreq_unlock(path);
                }
            }
            closedir(dir);
        }
    }
}

void unisoc_normal() {
    // GPU Frequency
    DIR *dir = opendir("/sys/class/devfreq");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".gpu")) {
                char gpu_path[MAX_PATH_LEN];
                snprintf(gpu_path, sizeof(gpu_path), "/sys/class/devfreq/%s", ent->d_name);
                devfreq_unlock(gpu_path);
                break;
            }
        }
        closedir(dir);
    }
}

void tensor_normal() {
    // Find GPU path
    DIR *dir = opendir("/sys/devices/platform");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".mali")) {
                char gpu_path[MAX_PATH_LEN];
                snprintf(gpu_path, sizeof(gpu_path), "/sys/devices/platform/%s", ent->d_name);
                
                char avail_path[MAX_PATH_LEN];
                snprintf(avail_path, sizeof(avail_path), "%s/available_frequencies", gpu_path);
                
                if (file_exists(avail_path)) {
                    long max_freq = get_max_freq(avail_path);
                    long min_freq = get_min_freq(avail_path);
                    
                    char max_freq_path[MAX_PATH_LEN];
                    snprintf(max_freq_path, sizeof(max_freq_path), "%s/scaling_max_freq", gpu_path);
                    write_ll(max_freq, max_freq_path);
                    
                    char min_freq_path[MAX_PATH_LEN];
                    snprintf(min_freq_path, sizeof(min_freq_path), "%s/scaling_min_freq", gpu_path);
                    write_ll(min_freq, min_freq_path);
                }
                break;
            }
        }
        closedir(dir);
    }
    
    // DRAM frequency
    if (DEVICE_MITIGATION == 0) {
        DIR *dir = opendir("/sys/class/devfreq");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (strstr(ent->d_name, "devfreq_mif")) {
                    char path[MAX_PATH_LEN];
                    snprintf(path, sizeof(path), "/sys/class/devfreq/%s", ent->d_name);
                    devfreq_unlock(path);
                }
            }
            closedir(dir);
        }
    }
}

// Device-specific powersave functions
void mediatek_powersave() {
    // Free FPSGO
    apply("0", "/sys/kernel/fpsgo/common/force_onoff");
    apply("0", "/sys/pnpmgr/fpsgo_boost/boost_enable");
    apply("1", "/sys/module/mtk_fpsgo/parameters/perfmgr_enable");
    
    // DDR Boost mode
    apply("1", "/sys/devices/platform/boot_dramboost/dramboost/dramboost");
    
    // EAS/HMP Switch
    apply("1", "/sys/devices/system/cpu/eas/enable");
    
    // MTK CPU Power mode to low power
    apply("1", "/proc/cpufreq/cpufreq_power_mode");
    
    // GPU Frequency
    if (file_exists("/proc/gpufreqv2")) {
        int min_gpufreq_index = mtk_gpufreq_minfreq_index("/proc/gpufreqv2/gpu_working_opp_table");
        apply_ll(min_gpufreq_index, "/proc/gpufreqv2/fix_target_opp_index");
    } else if (file_exists("/proc/gpufreq/gpufreq_opp_dump")) {
        FILE *fp = fopen("/proc/gpufreq/gpufreq_opp_dump", "r");
        if (fp) {
            char line[MAX_LINE_LEN];
            long min_freq = LONG_MAX;
            
            while (fgets(line, sizeof(line), fp)) {
                char *freq_str = strstr(line, "freq =");
                if (freq_str) {
                    long freq = atol(freq_str + 6);
                    if (freq < min_freq) min_freq = freq;
                }
            }
            fclose(fp);
            
            if (min_freq != LONG_MAX) {
                apply_ll(min_freq, "/proc/gpufreq/gpufreq_opp_freq");
            }
        }
    }
}

void snapdragon_powersave() {
    // GPU Frequency
    devfreq_min_perf("/sys/class/kgsl/kgsl-3d0/devfreq");
}

void exynos_powersave() {
    // GPU Frequency
    const char *gpu_path = "/sys/kernel/gpu";
    if (file_exists(gpu_path)) {
        char avail_path[MAX_PATH_LEN];
        snprintf(avail_path, sizeof(avail_path), "%s/gpu_available_frequencies", gpu_path);
        
        long freq = get_min_freq(avail_path);
        
        char min_clock_path[MAX_PATH_LEN];
        snprintf(min_clock_path, sizeof(min_clock_path), "%s/gpu_min_clock", gpu_path);
        apply_ll(freq, min_clock_path);
        
        char max_clock_path[MAX_PATH_LEN];
        snprintf(max_clock_path, sizeof(max_clock_path), "%s/gpu_max_clock", gpu_path);
        apply_ll(freq, max_clock_path);
    }
}

void unisoc_powersave() {
    // GPU Frequency
    DIR *dir = opendir("/sys/class/devfreq");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".gpu")) {
                char gpu_path[MAX_PATH_LEN];
                snprintf(gpu_path, sizeof(gpu_path), "/sys/class/devfreq/%s", ent->d_name);
                devfreq_min_perf(gpu_path);
                break;
            }
        }
        closedir(dir);
    }
}

void tensor_powersave() {
    // Find GPU path
    DIR *dir = opendir("/sys/devices/platform");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".mali")) {
                char gpu_path[MAX_PATH_LEN];
                snprintf(gpu_path, sizeof(gpu_path), "/sys/devices/platform/%s", ent->d_name);
                
                char avail_path[MAX_PATH_LEN];
                snprintf(avail_path, sizeof(avail_path), "%s/available_frequencies", gpu_path);
                
                if (file_exists(avail_path)) {
                    long freq = get_min_freq(avail_path);
                    
                    char min_freq_path[MAX_PATH_LEN];
                    snprintf(min_freq_path, sizeof(min_freq_path), "%s/scaling_min_freq", gpu_path);
                    apply_ll(freq, min_freq_path);
                    
                    char max_freq_path[MAX_PATH_LEN];
                    snprintf(max_freq_path, sizeof(max_freq_path), "%s/scaling_max_freq", gpu_path);
                    apply_ll(freq, max_freq_path);
                }
                break;
            }
        }
        closedir(dir);
    }
}

void read_configs() {
    char soc_path[MAX_PATH_LEN];
    snprintf(soc_path, sizeof(soc_path), "%s/soc_recognition", MODULE_CONFIG);
    SOC = read_int_from_file(soc_path);
    
    char lite_path[MAX_PATH_LEN];
    snprintf(lite_path, sizeof(lite_path), "%s/lite_mode", MODULE_CONFIG);
    LITE_MODE = read_int_from_file(lite_path);
    
    char ppm_path[MAX_PATH_LEN];
    snprintf(ppm_path, sizeof(ppm_path), "%s/ppm_policies_mediatek", MODULE_CONFIG);
    read_string_from_file(PPM_POLICY, sizeof(PPM_POLICY), ppm_path);
    
    char custom_gov_path[MAX_PATH_LEN];
    snprintf(custom_gov_path, sizeof(custom_gov_path), "%s/custom_default_cpu_gov", MODULE_CONFIG);
    if (file_exists(custom_gov_path)) {
        read_string_from_file(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), custom_gov_path);
    } else {
        char default_gov_path[MAX_PATH_LEN];
        snprintf(default_gov_path, sizeof(default_gov_path), "%s/default_cpu_gov", MODULE_CONFIG);
        read_string_from_file(DEFAULT_CPU_GOV, sizeof(DEFAULT_CPU_GOV), default_gov_path);
    }
    
    char mitigation_path[MAX_PATH_LEN];
    snprintf(mitigation_path, sizeof(mitigation_path), "%s/device_mitigation", MODULE_CONFIG);
    DEVICE_MITIGATION = read_int_from_file(mitigation_path);
}

// Main performance scripts
void perfcommon() {
    // Disable Kernel panic
    apply("0", "/proc/sys/kernel/panic");
    apply("0", "/proc/sys/vm/panic_on_oom");
    apply("0", "/proc/sys/kernel/panic_on_oops");
    apply("0", "/proc/sys/kernel/panic_on_warn");
    apply("0", "/proc/sys/kernel/softlockup_panic");
    
    // Sync to data
    sync();
    
    // I/O Tweaks
    DIR *dir = opendir("/sys/block");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] != '.') {
                char iostats_path[MAX_PATH_LEN];
                snprintf(iostats_path, sizeof(iostats_path), "/sys/block/%s/queue/iostats", ent->d_name);
                apply("0", iostats_path);
                
                char random_path[MAX_PATH_LEN];
                snprintf(random_path, sizeof(random_path), "/sys/block/%s/queue/add_random", ent->d_name);
                apply("0", random_path);
            }
        }
        closedir(dir);
    }
    
    // Networking tweaks
    const char *algorithms[] = {"bbr3", "bbr2", "bbrplus", "bbr", "westwood", "cubic"};
    for (int i = 0; i < 6; i++) {
        snprintf(cmd, sizeof(cmd), "grep -q \"%s\" /proc/sys/net/ipv4/tcp_available_congestion_control", algorithms[i]);
        if (system(cmd) == 0) {
            apply(algorithms[i], "/proc/sys/net/ipv4/tcp_congestion_control");
            break;
        }
    }
    
    apply("1", "/proc/sys/net/ipv4/tcp_sack");
    apply("1", "/proc/sys/net/ipv4/tcp_fack");
    apply("2", "/proc/sys/net/ipv4/tcp_ecn");
    apply("1", "/proc/sys/net/ipv4/tcp_window_scaling");
    apply("1", "/proc/sys/net/ipv4/tcp_moderate_rcvbuf");
    apply("3", "/proc/sys/net/ipv4/tcp_fastopen");
    
    // Limit max perf event processing time
    apply("3", "/proc/sys/kernel/perf_cpu_time_max_percent");
    
    // Disable schedstats
    apply("0", "/proc/sys/kernel/sched_schedstats");
    
    // Disable Oppo/Realme cpustats
    apply("0", "/proc/sys/kernel/task_cpustats_enable");
        
    // Update /proc/stat less often
    apply("15", "/proc/sys/vm/stat_interval");
    
    // VM Writeback Control
    apply("0", "/proc/sys/vm/page-cluster");
    apply("15", "/proc/sys/vm/stat_interval");
    apply("80", "/proc/sys/vm/overcommit_ratio");
    
    // Disable Sched auto group
    apply("0", "/proc/sys/kernel/sched_autogroup_enabled");
    
    // Enable CRF
    apply("1", "/proc/sys/kernel/sched_child_runs_first");
    
    // Disable SPI CRC
    apply("0", "/sys/module/mmc_core/parameters/use_spi_crc");
    
    // Disable OnePlus opchain
    apply("0", "/sys/module/opchain/parameters/chain_on");
    
    // Disable Oplus bloats
    apply("0", "/sys/module/cpufreq_bouncing/parameters/enable");
    apply("0", "/proc/task_info/task_sched_info/task_sched_info_enable");
    apply("0", "/proc/oplus_scheduler/sched_assist/sched_assist_enabled");
    
    // Reduce kernel log noise
    apply("0", "/proc/sys/kernel/printk");
    apply("off", "/proc/sys/kernel/printk_devkmsg");
    
    // Report max CPU capabilities
    apply("libunity.so, libil2cpp.so, libmain.so, libUE4.so, libgodot_android.so, libgdx.so, libgdx-box2d.so, libminecraftpe.so, libLive2DCubismCore.so, libyuzu-android.so, libryujinx.so, libcitra-android.so, libhdr_pro_engine.so, libandroidx.graphics.path.so, libeffect.so", "/proc/sys/kernel/sched_lib_name");
    apply("255", "/proc/sys/kernel/sched_lib_mask_force");
        
    // Set thermal governor to step_wise
    DIR *thermal_dir = opendir("/sys/class/thermal");
    if (thermal_dir) {
        struct dirent *ent;
        while ((ent = readdir(thermal_dir)) != NULL) {
            if (strstr(ent->d_name, "thermal_zone")) {
                char policy_path[MAX_PATH_LEN];
                snprintf(policy_path, sizeof(policy_path), "/sys/class/thermal/%s/policy", ent->d_name);
                apply("step_wise", policy_path);
            }
        }
        closedir(thermal_dir);
    }
}

void performance_profile() {
    // Enable Do not Disturb
    char dnd_path[MAX_PATH_LEN];
    snprintf(dnd_path, sizeof(dnd_path), "%s/dnd_gameplay", MODULE_CONFIG);
    if (read_int_from_file(dnd_path) == 1) {
        set_dnd(1);
    }
                
    // Disable battery saver module
    if (file_exists("/sys/module/battery_saver/parameters/enabled")) {
        FILE *fp = fopen("/sys/module/battery_saver/parameters/enabled", "r");
        if (fp) {
            char line[10];
            if (fgets(line, sizeof(line), fp)) {
                if (atoi(line) != 0) {
                    apply("0", "/sys/module/battery_saver/parameters/enabled");
                } else if (line[0] == 'Y' || line[0] == 'y') {
                    apply("N", "/sys/module/battery_saver/parameters/enabled");
                }
            }
            fclose(fp);
        }
    }
    
    apply("N", "/sys/module/workqueue/parameters/power_efficient");
    
    // Network Tweak
    apply("15", "/proc/sys/net/ipv4/tcp_fin_timeout");
    apply("1", "/proc/sys/net/ipv4/tcp_low_latency");
    apply("0", "/proc/sys/net/ipv4/tcp_slow_start_after_idle");
    apply("0", "/proc/sys/net/ipv4/tcp_timestamps");
    
    // Disable split lock mitigation
    apply("0", "/proc/sys/kernel/split_lock_mitigate");
    
    // VM Writeback Control
    apply("5", "/proc/sys/vm/dirty_background_ratio");
    apply("15", "/proc/sys/vm/dirty_ratio");
    apply("1500", "/proc/sys/vm/dirty_expire_centisecs");
    apply("1500", "/proc/sys/vm/dirty_writeback_centisecs");
    
    // Sched Boost
    apply("2", "/proc/sys/kernel/sched_boost");
          
    // Improve real time latencies
    apply("32", "/proc/sys/kernel/sched_nr_migrate");
    
    // Tweaking scheduler
    apply("15", "proc/sys/kernel/sched_min_task_util_for_boost");
    apply("8", "/proc/sys/kernel/sched_min_task_util_for_colocation");
    apply("50000",  "/proc/sys/kernel/sched_migration_cost_ns");
    apply("800000", "/proc/sys/kernel/sched_min_granularity_ns");
    apply("900000", "/proc/sys/kernel/sched_wakeup_granularity_ns");
      
    if (file_exists("/sys/kernel/debug/sched_features")) {
        apply("NEXT_BUDDY", "/sys/kernel/debug/sched_features");
        apply("NO_TTWU_QUEUE", "/sys/kernel/debug/sched_features");
    }
    
    if (file_exists("/dev/stune/top-app/schedtune.prefer_idle")) {
        apply("1", "/dev/stune/top-app/schedtune.prefer_idle");
        apply("1", "/dev/stune/top-app/schedtune.boost");
    }
        
    // Oppo/Oplus/Realme Touchpanel
    const char *tp_path = "/proc/touchpanel";
    if (file_exists(tp_path)) {
        apply("1", "/proc/touchpanel/game_switch_enable");
        apply("0", "/proc/touchpanel/oplus_tp_limit_enable");
        apply("0", "/proc/touchpanel/oppo_tp_limit_enable");
        apply("1", "/proc/touchpanel/oplus_tp_direction");
        apply("1", "/proc/touchpanel/oppo_tp_direction");
    }
    
    // Memory tweak
    apply("20", "/proc/sys/vm/swappiness");
    apply("60", "/proc/sys/vm/vfs_cache_pressure");
    apply("30", "/proc/sys/vm/compaction_proactiveness");
    
    // eMMC and UFS frequency
    DIR *dir = opendir("/sys/class/devfreq");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            char *name = ent->d_name;
            if (strstr(name, ".ufshc") || strstr(name, "mmc")) {
                char path[MAX_PATH_LEN];
                snprintf(path, sizeof(path), "/sys/class/devfreq/%s", name);
                
                if (LITE_MODE == 1) {
                    devfreq_mid_perf(path);
                } else {
                    devfreq_max_perf(path);
                }
            }
        }
        closedir(dir);
    }
    
    // Set CPU governor
    if (LITE_MODE == 0 && DEVICE_MITIGATION == 0) {
        change_cpu_gov("performance");
    } else {
        change_cpu_gov(DEFAULT_CPU_GOV);
    }
    
    // Force CPU to highest possible frequency
    if (file_exists("/proc/ppm")) {
        cpufreq_ppm_max_perf();
    } else {
        cpufreq_max_perf();
    }
    
    // I/O Tweaks
    const char *block_devs[] = {"mmcblk0", "mmcblk1"};
    for (int i = 0; i < 2; i++) {
        char read_ahead_path[MAX_PATH_LEN];
        snprintf(read_ahead_path, sizeof(read_ahead_path), "/sys/block/%s/queue/read_ahead_kb", block_devs[i]);
        if (file_exists(read_ahead_path)) {
            apply("32", read_ahead_path);
        }
        
        char nr_requests_path[MAX_PATH_LEN];
        snprintf(nr_requests_path, sizeof(nr_requests_path), "/sys/block/%s/queue/nr_requests", block_devs[i]);
        if (file_exists(nr_requests_path)) {
            apply("32", nr_requests_path);
        }
    }
    
    // Process SD cards
    dir = opendir("/sys/block");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, "sd")) {
                char read_ahead_path[MAX_PATH_LEN];
                snprintf(read_ahead_path, sizeof(read_ahead_path), "/sys/block/%s/queue/read_ahead_kb", ent->d_name);
                if (file_exists(read_ahead_path)) {
                    apply("32", read_ahead_path);
                }
                
                char nr_requests_path[MAX_PATH_LEN];
                snprintf(nr_requests_path, sizeof(nr_requests_path), "/sys/block/%s/queue/nr_requests", ent->d_name);
                if (file_exists(nr_requests_path)) {
                    apply("32", nr_requests_path);
                }
            }
        }
        closedir(dir);
    }
        
    // SOC-specific performance tweaks
    switch (SOC) {
        case 1: mediatek_performance(); break;
        case 2: snapdragon_performance(); break;
        case 3: exynos_performance(); break;
        case 4: unisoc_performance(); break;
        case 5: tensor_performance(); break;
    }
    
    // Drop caches
    apply("3", "/proc/sys/vm/drop_caches");
}

void normal_profile() {
    char dnd_path[MAX_PATH_LEN];
    snprintf(dnd_path, sizeof(dnd_path), "%s/dnd_gameplay", MODULE_CONFIG);
    if (read_int_from_file(dnd_path) == 1) {
        set_dnd(0);
    }
    
    // Disable battery saver module
    if (file_exists("/sys/module/battery_saver/parameters/enabled")) {
        FILE *fp = fopen("/sys/module/battery_saver/parameters/enabled", "r");
        if (fp) {
            char line[10];
            if (fgets(line, sizeof(line), fp)) {
                if (atoi(line) != 0) {
                    apply("0", "/sys/module/battery_saver/parameters/enabled");
                } else if (line[0] == 'Y' || line[0] == 'y') {
                    apply("N", "/sys/module/battery_saver/parameters/enabled");
                }
            }
            fclose(fp);
        }
    }
    
    apply("N", "/sys/module/workqueue/parameters/power_efficient");
    
    // Network Tweak
    apply("15", "/proc/sys/net/ipv4/tcp_fin_timeout");
    apply("1", "/proc/sys/net/ipv4/tcp_low_latency");
    apply("1", "/proc/sys/net/ipv4/tcp_slow_start_after_idle");
    apply("1", "/proc/sys/net/ipv4/tcp_timestamps");
    
    // Enable split lock mitigation
    apply("1", "/proc/sys/kernel/split_lock_mitigate");
    
    // VM Writeback Control
    apply("10", "/proc/sys/vm/dirty_background_ratio");
    apply("25", "/proc/sys/vm/dirty_ratio");
    apply("3000", "/proc/sys/vm/dirty_expire_centisecs");
    apply("3000", "/proc/sys/vm/dirty_writeback_centisecs");
    
    // Sched Boost
    apply("1", "/proc/sys/kernel/sched_boost");
    
    // Improve real time latencies
    apply("16", "/proc/sys/kernel/sched_nr_migrate");
    
    // Tweaking scheduler
    apply("25", "proc/sys/kernel/sched_min_task_util_for_boost");
    apply("15", "/proc/sys/kernel/sched_min_task_util_for_colocation");
    apply("100000", "/proc/sys/kernel/sched_migration_cost_ns");
    apply("1200000", "/proc/sys/kernel/sched_min_granularity_ns");
    apply("2000000", "/proc/sys/kernel/sched_wakeup_granularity_ns");
    
    if (file_exists("/sys/kernel/debug/sched_features")) {
        apply("NEXT_BUDDY", "/sys/kernel/debug/sched_features");
        apply("TTWU_QUEUE", "/sys/kernel/debug/sched_features");
    }
    
    if (file_exists("/dev/stune/top-app/schedtune.prefer_idle")) {
        apply("0", "/dev/stune/top-app/schedtune.prefer_idle");
        apply("1", "/dev/stune/top-app/schedtune.boost");
    }
    
    // Oppo/Oplus/Realme Touchpanel
    const char *tp_path = "/proc/touchpanel";
    if (file_exists(tp_path)) {
        apply("0", "/proc/touchpanel/game_switch_enable");
        apply("1", "/proc/touchpanel/oplus_tp_limit_enable");
        apply("1", "/proc/touchpanel/oppo_tp_limit_enable");
        apply("0", "/proc/touchpanel/oplus_tp_direction");
        apply("0", "/proc/touchpanel/oppo_tp_direction");
    }
    
    // Memory Tweaks
    apply("40", "/proc/sys/vm/swappiness");
    apply("80", "/proc/sys/vm/vfs_cache_pressure");
    apply("40", "/proc/sys/vm/compaction_proactiveness");
    
    // eMMC and UFS frequency
    DIR *dir = opendir("/sys/class/devfreq");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            char *name = ent->d_name;
            if (strstr(name, ".ufshc") || strstr(name, "mmc")) {
                char path[MAX_PATH_LEN];
                snprintf(path, sizeof(path), "/sys/class/devfreq/%s", name);
                devfreq_unlock(path);
            }
        }
        closedir(dir);
    }
    
    // Restore CPU settings
    change_cpu_gov(DEFAULT_CPU_GOV);
    if (file_exists("/proc/ppm")) {
        cpufreq_ppm_unlock();
    } else {
        cpufreq_unlock();
    }
    
    // I/O Tweaks
    const char *block_devs[] = {"mmcblk0", "mmcblk1"};
    for (int i = 0; i < 2; i++) {
        char read_ahead_path[MAX_PATH_LEN];
        snprintf(read_ahead_path, sizeof(read_ahead_path), "/sys/block/%s/queue/read_ahead_kb", block_devs[i]);
        if (file_exists(read_ahead_path)) {
            apply("64", read_ahead_path);
        }
        
        char nr_requests_path[MAX_PATH_LEN];
        snprintf(nr_requests_path, sizeof(nr_requests_path), "/sys/block/%s/queue/nr_requests", block_devs[i]);
        if (file_exists(nr_requests_path)) {
            apply("64", nr_requests_path);
        }
    }
    
    // Process SD cards
    dir = opendir("/sys/block");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, "sd")) {
                char read_ahead_path[MAX_PATH_LEN];
                snprintf(read_ahead_path, sizeof(read_ahead_path), "/sys/block/%s/queue/read_ahead_kb", ent->d_name);
                if (file_exists(read_ahead_path)) {
                    apply("128", read_ahead_path);
                }
                
                char nr_requests_path[MAX_PATH_LEN];
                snprintf(nr_requests_path, sizeof(nr_requests_path), "/sys/block/%s/queue/nr_requests", ent->d_name);
                if (file_exists(nr_requests_path)) {
                    apply("64", nr_requests_path);
                }
            }
        }
        closedir(dir);
    }
       
    // SOC-specific normal tweaks
    switch (SOC) {
        case 1: mediatek_normal(); break;
        case 2: snapdragon_normal(); break;
        case 3: exynos_normal(); break;
        case 4: unisoc_normal(); break;
        case 5: tensor_normal(); break;
    }
}

void powersave_profile() {
    char dnd_path[MAX_PATH_LEN];
    snprintf(dnd_path, sizeof(dnd_path), "%s/dnd_gameplay", MODULE_CONFIG);
    if (read_int_from_file(dnd_path) == 1) {
        set_dnd(0);
    }    

    // Enable battery saver module
    if (file_exists("/sys/module/battery_saver/parameters/enabled")) {
        FILE *fp = fopen("/sys/module/battery_saver/parameters/enabled", "r");
        if (fp) {
            char line[10];
            if (fgets(line, sizeof(line), fp)) {
                if (atoi(line) == 0) {
                    apply("1", "/sys/module/battery_saver/parameters/enabled");
                } else if (line[0] == 'N' || line[0] == 'n') {
                    apply("Y", "/sys/module/battery_saver/parameters/enabled");
                }
            }
            fclose(fp);
        }
    }
    
    apply("Y", "/sys/module/workqueue/parameters/power_efficient");
    
    // Network tweak
    apply("25", "/proc/sys/net/ipv4/tcp_fin_timeout");
    apply("0", "/proc/sys/net/ipv4/tcp_low_latency");
    apply("1", "/proc/sys/net/ipv4/tcp_slow_start_after_idle");
    apply("0", "/proc/sys/net/ipv4/tcp_timestamps");
                    
    // Enable split lock mitigation
    apply("1", "/proc/sys/kernel/split_lock_mitigate");
    
    // VM Writeback Control
    apply("20", "/proc/sys/vm/dirty_background_ratio");
    apply("40", "/proc/sys/vm/dirty_ratio");
    apply("6000", "/proc/sys/vm/dirty_writeback_centisecs");
    apply("6000", "/proc/sys/vm/dirty_expire_centisecs");
    
    // Sched Boost
    apply("0", "/proc/sys/kernel/sched_boost");
    
    // Improve real time latencies
    apply("8", "/proc/sys/kernel/sched_nr_migrate");
    
    // Tweaking scheduler
    apply("45", "proc/sys/kernel/sched_min_task_util_for_boost");
    apply("30", "/proc/sys/kernel/sched_min_task_util_for_colocation");
    apply("200000", "/proc/sys/kernel/sched_migration_cost_ns");
    apply("2000000", "/proc/sys/kernel/sched_min_granularity_ns");
    apply("3000000", "/proc/sys/kernel/sched_wakeup_granularity_ns");
    
    if (file_exists("/sys/kernel/debug/sched_features")) {
        apply("NO_NEXT_BUDDY", "/sys/kernel/debug/sched_features");
        apply("TTWU_QUEUE", "/sys/kernel/debug/sched_features");
    }
    
    if (file_exists("/dev/stune/top-app/schedtune.prefer_idle")) {
        apply("1", "/dev/stune/top-app/schedtune.prefer_idle");
        apply("0", "/dev/stune/top-app/schedtune.boost");
    }
    
    // Oppo/Oplus/Realme Touchpanel
    const char *tp_path = "/proc/touchpanel";
    if (file_exists(tp_path)) {
        apply("0", "/proc/touchpanel/game_switch_enable");
        apply("1", "/proc/touchpanel/oplus_tp_limit_enable");
        apply("1", "/proc/touchpanel/oppo_tp_limit_enable");
        apply("0", "/proc/touchpanel/oplus_tp_direction");
        apply("0", "/proc/touchpanel/oppo_tp_direction");
    }
    
    // Memory Tweaks
    apply("60", "/proc/sys/vm/swappiness");
    apply("100", "/proc/sys/vm/vfs_cache_pressure");
    apply("10", "/proc/sys/vm/compaction_proactiveness");
    
    // eMMC and UFS frequency
    DIR *dir = opendir("/sys/class/devfreq");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            char *name = ent->d_name;
            if (strstr(name, ".ufshc") || strstr(name, "mmc")) {
                char path[MAX_PATH_LEN];
                snprintf(path, sizeof(path), "/sys/class/devfreq/%s", name);
                devfreq_min_perf(path);
            }
        }
        closedir(dir);
    }
    
    // CPU governor for powersave
    char powersave_gov_path[MAX_PATH_LEN];
    snprintf(powersave_gov_path, sizeof(powersave_gov_path), "%s/powersave_cpu_gov", MODULE_CONFIG);
    char powersave_gov[50];
    read_string_from_file(powersave_gov, sizeof(powersave_gov), powersave_gov_path);
    change_cpu_gov(powersave_gov);
    
    // I/O Tweaks
    const char *block_devs[] = {"mmcblk0", "mmcblk1"};
    for (int i = 0; i < 2; i++) {
        char read_ahead_path[MAX_PATH_LEN];
        snprintf(read_ahead_path, sizeof(read_ahead_path), "/sys/block/%s/queue/read_ahead_kb", block_devs[i]);
        if (file_exists(read_ahead_path)) {
            apply("16", read_ahead_path);
        }
        
        char nr_requests_path[MAX_PATH_LEN];
        snprintf(nr_requests_path, sizeof(nr_requests_path), "/sys/block/%s/queue/nr_requests", block_devs[i]);
        if (file_exists(nr_requests_path)) {
            apply("16", nr_requests_path);
        }
    }

    // Process SD cards
    dir = opendir("/sys/block");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, "sd")) {
                char read_ahead_path[MAX_PATH_LEN];
                snprintf(read_ahead_path, sizeof(read_ahead_path), "/sys/block/%s/queue/read_ahead_kb", ent->d_name);
                if (file_exists(read_ahead_path)) {
                    apply("128", read_ahead_path);
                }
                
                char nr_requests_path[MAX_PATH_LEN];
                snprintf(nr_requests_path, sizeof(nr_requests_path), "/sys/block/%s/queue/nr_requests", ent->d_name);
                if (file_exists(nr_requests_path)) {
                    apply("64", nr_requests_path);
                }
            }
        }
        closedir(dir);
    }
            
    // SOC-specific powersave tweaks
    switch (SOC) {
        case 1: mediatek_powersave(); break;
        case 2: snapdragon_powersave(); break;
        case 3: exynos_powersave(); break;
        case 4: unisoc_powersave(); break;
        case 5: tensor_powersave(); break;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <mode>\n", argv[0]);
        printf("Modes:\n");
        printf("  0 - Common tweaks\n");
        printf("  1 - Performance profile\n");
        printf("  2 - Normal profile\n");
        printf("  3 - Powersave profile\n");
        return 1;
    }
    
    int mode = atoi(argv[1]);
    
    // Read configuration files
    read_configs();
    
    // Execute based on mode
    switch (mode) {
        case 0:
            perfcommon();
            break;
        case 1:
            perfcommon();
            performance_profile();
            break;
        case 2:
            perfcommon();
            normal_profile();
            break;
        case 3:
            perfcommon();
            powersave_profile();
            break;
        default:
            printf("Invalid mode: %d\n", mode);
            return 1;
    }
    return 0;
}
