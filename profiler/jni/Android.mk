LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := nusantara_profiler
LOCAL_SRC_FILES := ../src/nusantara_profiler.c

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
    -Wno-sign-compare

LOCAL_LDFLAGS := \
    -flto \
    -Wl,-z,relro \
    -Wl,-z,now

LOCAL_LDLIBS += -llog

include $(BUILD_EXECUTABLE)
