LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := sys.nusaservice
LOCAL_SRC_FILES := \
    ../main.c \
    ../src/cmd_utils.c \
    ../src/nusantara_log.c \
    ../src/nusantara_profiler.c \
    ../src/file_utils.c \
    ../src/process_utils.c \
    ../src/misc_utils.c \
    ../src/preload_function.c \
    ../src/mlbb_handler.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include

LOCAL_CFLAGS := \
    -DNDEBUG \
    -O2 \
    -std=c23 \
    -fPIC \
    -D_GNU_SOURCE \
    -flto \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -Wno-sign-compare \
    -Wno-missing-field-initializers

LOCAL_LDFLAGS := \
    -flto \
    -Wl,-z,relro \
    -Wl,-z,now

LOCAL_LDLIBS += -llog

include $(BUILD_EXECUTABLE)