LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := native-interface

LOCAL_SRC_FILES :=	video.c				\
					adpcm.c				\
					audio.c				\
					sip.c				\
					network.c			\
					circular_queue.c	\
					native_interface.c

LOCAL_SHARED_LIBRARIES := libosip		\
						  libexosip		\
						  libavcodec	\
						  libavutil		\
						  libswscale

# LOCAL_SHARED_LIBRARIES := libavcodec	\
						  # libavutil		\
						  # libswscale

LOCAL_LDLIBS += -llog
LOCAL_LDLIBS += -lOpenSLES
LOCAL_LDLIBS += -landroid

include $(BUILD_SHARED_LIBRARY)
$(call import-add-path, $(LOCAL_PATH))
$(call import-module, libffmpeg)
$(call import-module, libosip)
$(call import-module, libexosip)