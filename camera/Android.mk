LOCAL_PATH := $(call my-dir)

MM_V4L2_DRIVER_LIST := msm8994 msm8992

ifneq (,$(filter $(MM_V4L2_DRIVER_LIST),$(TARGET_BOARD_PLATFORM)))
  ifneq ($(strip $(USE_CAMERA_STUB)),true)
    ifneq ($(BUILD_TINY_ANDROID),true)
      CAF_CAMERA_ROOT := $(LOCAL_PATH)
      include $(CAF_CAMERA_ROOT)/mm-image-codec/qomx_core/Android.mk
      include $(CAF_CAMERA_ROOT)/QCamera2/stack/mm-camera-interface/Android.mk
      include $(CAF_CAMERA_ROOT)/QCamera2/stack/mm-jpeg-interface/Android.mk
      include $(CAF_CAMERA_ROOT)/QCamera2/Android.mk
    endif
  endif
endif
