LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := nusantara_profiler
LOCAL_SRC_FILES := ../src/nusantara_profiler.c

LOCAL_CFLAGS := \
    -DNDEBUG \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -pedantic-errors \
    -Wpedantic \
    -O2 \
    -std=gnu17 \
    -fPIC \
    -D_GNU_SOURCE \
    -ffunction-sections \
    -fdata-sections

LOCAL_LDFLAGS := \
    -Wl,--gc-sections \
    -Wl,-z,relro \
    -Wl,-z,now

# Android log lib
LOCAL_LDLIBS += -llog

include $(BUILD_EXECUTABLE)
