LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := nusantara_profiler
LOCAL_SRC_FILES := ../src/nusantara_profiler.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_CFLAGS := -DNDEBUG -Wall -Wextra -Werror \
                -pedantic-errors -Wpedantic \
                -O2 -std=c23 -fPIC -flto

LOCAL_LDFLAGS := -flto
LOCAL_LDLIBS  += -llog  

include $(BUILD_EXECUTABLE)
