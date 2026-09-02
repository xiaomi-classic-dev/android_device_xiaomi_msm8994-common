/* Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE DISCLAIMED.
 */

#define LOG_TAG "QCameraFlash"

#include <errno.h>
#include <fcntl.h>
#include <media/msm_cam_sensor.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <utils/Log.h>

#include "HAL3/QCamera3HWI.h"
#include "QCameraFlash.h"

namespace qcamera {

static const int kTorchCurrentMa = 200;

QCameraFlash& QCameraFlash::getInstance()
{
    static QCameraFlash instance;
    return instance;
}

QCameraFlash::QCameraFlash() : mCallbacks(NULL)
{
    pthread_mutex_init(&mLock, NULL);
    memset(mFlashOn, 0, sizeof(mFlashOn));
    memset(mCameraOpen, 0, sizeof(mCameraOpen));
    for (int i = 0; i < MM_CAMERA_MAX_NUM_SENSORS; ++i) {
        mFlashFds[i] = -1;
    }
}

QCameraFlash::~QCameraFlash()
{
    pthread_mutex_lock(&mLock);
    for (int i = 0; i < MM_CAMERA_MAX_NUM_SENSORS; ++i) {
        deinitFlashLocked(i);
    }
    pthread_mutex_unlock(&mLock);
    pthread_mutex_destroy(&mLock);
}

int32_t QCameraFlash::registerCallbacks(
        const camera_module_callbacks_t *callbacks)
{
    pthread_mutex_lock(&mLock);
    mCallbacks = callbacks;
    pthread_mutex_unlock(&mLock);
    return 0;
}

bool QCameraFlash::findFlashNode(char *path, size_t pathSize)
{
    for (int i = 0; i < 64; ++i) {
        char sysfsPath[128];
        snprintf(sysfsPath, sizeof(sysfsPath),
                "/sys/class/video4linux/v4l-subdev%d/name", i);
        int fd = open(sysfsPath, O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            continue;
        }

        char name[64] = {};
        ssize_t len = read(fd, name, sizeof(name) - 1);
        close(fd);
        if (len > 0 && strstr(name, "msm_camera_flash") != NULL) {
            snprintf(path, pathSize, "/dev/v4l-subdev%d", i);
            return true;
        }
    }
    return false;
}

int32_t QCameraFlash::initFlashLocked(int cameraId)
{
    if (!QCamera3HardwareInterface::hasFlashUnit(cameraId)) {
        return -ENOSYS;
    }
    if (mCameraOpen[cameraId]) {
        return -EBUSY;
    }
    if (mFlashFds[cameraId] >= 0) {
        return 0;
    }

    char flashPath[64];
    if (!findFlashNode(flashPath, sizeof(flashPath))) {
        ALOGE("Unable to locate msm_camera_flash subdevice");
        return -ENODEV;
    }

    int fd = open(flashPath, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        ALOGE("Unable to open %s: %s", flashPath, strerror(errno));
        return -errno;
    }

    struct msm_flash_cfg_data_t cfg = {};
    struct msm_flash_init_info_t initInfo = {};
    initInfo.flash_driver_type = FLASH_DRIVER_DEFAULT;
    cfg.cfg.flash_init_info = &initInfo;
    cfg.cfg_type = CFG_FLASH_INIT;
    if (ioctl(fd, VIDIOC_MSM_FLASH_CFG, &cfg) < 0) {
        int rc = -errno;
        ALOGE("Unable to initialize flash: %s", strerror(errno));
        close(fd);
        return rc;
    }

    mFlashFds[cameraId] = fd;
    usleep(5000);
    return 0;
}

int32_t QCameraFlash::setFlashModeLocked(int cameraId, bool on)
{
    if (mFlashFds[cameraId] < 0) {
        return -EINVAL;
    }
    if (mFlashOn[cameraId] == on) {
        return 0;
    }

    struct msm_flash_cfg_data_t cfg = {};
    cfg.cfg_type = on ? CFG_FLASH_LOW : CFG_FLASH_OFF;
    for (int i = 0; i < MAX_LED_TRIGGERS; ++i) {
        cfg.flash_current[i] = kTorchCurrentMa;
    }
    if (ioctl(mFlashFds[cameraId], VIDIOC_MSM_FLASH_CFG, &cfg) < 0) {
        int rc = -errno;
        ALOGE("Unable to set flash mode %d: %s", on, strerror(errno));
        return rc;
    }

    mFlashOn[cameraId] = on;
    return 0;
}

int32_t QCameraFlash::deinitFlashLocked(int cameraId)
{
    if (mFlashFds[cameraId] < 0) {
        mFlashOn[cameraId] = false;
        return 0;
    }

    int32_t rc = setFlashModeLocked(cameraId, false);
    struct msm_flash_cfg_data_t cfg = {};
    cfg.cfg_type = CFG_FLASH_RELEASE;
    if (ioctl(mFlashFds[cameraId], VIDIOC_MSM_FLASH_CFG, &cfg) < 0 && rc == 0) {
        rc = -errno;
    }
    close(mFlashFds[cameraId]);
    mFlashFds[cameraId] = -1;
    mFlashOn[cameraId] = false;
    return rc;
}

int32_t QCameraFlash::setTorchMode(int cameraId, bool on)
{
    if (cameraId < 0 || cameraId >= MM_CAMERA_MAX_NUM_SENSORS) {
        return -EINVAL;
    }

    pthread_mutex_lock(&mLock);
    int32_t rc;
    if (on) {
        rc = initFlashLocked(cameraId);
        if (rc == 0) {
            rc = setFlashModeLocked(cameraId, true);
        }
    } else {
        rc = deinitFlashLocked(cameraId);
    }
    const camera_module_callbacks_t *callbacks = mCallbacks;
    pthread_mutex_unlock(&mLock);

    if (rc == 0 && callbacks != NULL &&
            callbacks->torch_mode_status_change != NULL) {
        char id[16];
        snprintf(id, sizeof(id), "%d", cameraId);
        callbacks->torch_mode_status_change(callbacks, id,
                on ? TORCH_MODE_STATUS_AVAILABLE_ON :
                     TORCH_MODE_STATUS_AVAILABLE_OFF);
    }
    return rc;
}

int32_t QCameraFlash::reserveFlashForCamera(int cameraId)
{
    if (cameraId < 0 || cameraId >= MM_CAMERA_MAX_NUM_SENSORS) {
        return -EINVAL;
    }

    pthread_mutex_lock(&mLock);
    bool hasFlash = QCamera3HardwareInterface::hasFlashUnit(cameraId);
    if (hasFlash) {
        deinitFlashLocked(cameraId);
    }
    mCameraOpen[cameraId] = true;
    const camera_module_callbacks_t *callbacks = mCallbacks;
    pthread_mutex_unlock(&mLock);

    if (hasFlash && callbacks != NULL &&
            callbacks->torch_mode_status_change != NULL) {
        char id[16];
        snprintf(id, sizeof(id), "%d", cameraId);
        callbacks->torch_mode_status_change(callbacks, id,
                TORCH_MODE_STATUS_NOT_AVAILABLE);
    }
    return 0;
}

int32_t QCameraFlash::releaseFlashFromCamera(int cameraId)
{
    if (cameraId < 0 || cameraId >= MM_CAMERA_MAX_NUM_SENSORS) {
        return -EINVAL;
    }

    pthread_mutex_lock(&mLock);
    mCameraOpen[cameraId] = false;
    bool hasFlash = QCamera3HardwareInterface::hasFlashUnit(cameraId);
    const camera_module_callbacks_t *callbacks = mCallbacks;
    pthread_mutex_unlock(&mLock);

    if (hasFlash && callbacks != NULL &&
            callbacks->torch_mode_status_change != NULL) {
        char id[16];
        snprintf(id, sizeof(id), "%d", cameraId);
        callbacks->torch_mode_status_change(callbacks, id,
                TORCH_MODE_STATUS_AVAILABLE_OFF);
    }
    return 0;
}

} // namespace qcamera
