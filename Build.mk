LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
WITH_DEBUG := true
LOCAL_MODULE = TakePicture
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(OUT_DEVICE_BINRARY_DIR)
LOCAL_SRC_FILES := Main.cpp TakePicture.cpp UploadImageAI.cpp cJSON.c

# LOCAL_CFLAGS += -fPIC -D_GNU_SOURCE=1 -DHAVE_PTHREADS -DHAVE_ANDROID_OS
LOCAL_LDFLAGS := -rpath-link=$(OUT_DEVICE_SHARED_DIR)
LOCAL_LDLIBS := -lstdc++ -lrt -lpthread -lm -lc

LOCAL_DEPANNER_MODULES += \
	libdlog \
	libcutils \
	libutils \
	libcamera_metadata \
	libcamera_client \
	libcameraservice \
	libsettingdb \
	libled

LOCAL_SHARED_LIBRARIES += \
	libdlog.so \
	libcutils.so \
	libutils.so \
	libcamera_metadata.so \
	libcamera_client.so \
	libcameraservice.so \
	libsettingdb.so \
        libled.so

include $(BUILD_EXECUTABLE)
