LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := nusantara_profiler
LOCAL_SRC_FILES := ../src/nusantara_profiler.c

LOCAL_CFLAGS := \
    -DNDEBUG \
    -O2 \
    -std=gnu17 \
    -fPIC \
    -D_GNU_SOURCE \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -Wno-sign-compare

LOCAL_LDFLAGS := \
    -Wl,-z,relro \
    -Wl,-z,now
    
LOCAL_LDLIBS  += -llog  

include $(BUILD_EXECUTABLE)
