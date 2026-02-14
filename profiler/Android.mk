LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := nusantara_profiler
LOCAL_SRC_FILES := nusantara_profiler.c
LOCAL_CFLAGS := -DNDEBUG \
                -Wall -Wextra \
                -O3 \
                -std=c17 \
                -ffunction-sections \
                -fdata-sections \
                -fomit-frame-pointer \
                -fno-unwind-tables \
                -fno-asynchronous-unwind-tables \
                -flto

LOCAL_LDFLAGS := -Wl,--gc-sections \
                 -Wl,--strip-all \
                 -Wl,-z,relro,-z,now \
                 -flto

LOCAL_LDLIBS := -llog

include $(BUILD_EXECUTABLE)
