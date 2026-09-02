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

#ifndef __QCAMERA_FLASH_H__
#define __QCAMERA_FLASH_H__

#include <hardware/camera_common.h>

extern "C" {
#include "mm_camera_interface.h"
}

namespace qcamera {

class QCameraFlash {
public:
    static QCameraFlash& getInstance();

    int32_t registerCallbacks(const camera_module_callbacks_t *callbacks);
    int32_t setTorchMode(int cameraId, bool on);
    int32_t reserveFlashForCamera(int cameraId);
    int32_t releaseFlashFromCamera(int cameraId);

private:
    QCameraFlash();
    ~QCameraFlash();
    QCameraFlash(const QCameraFlash&);
    QCameraFlash& operator=(const QCameraFlash&);

    int32_t initFlashLocked(int cameraId);
    int32_t setFlashModeLocked(int cameraId, bool on);
    int32_t deinitFlashLocked(int cameraId);
    static bool findFlashNode(char *path, size_t pathSize);

    pthread_mutex_t mLock;
    const camera_module_callbacks_t *mCallbacks;
    int mFlashFds[MM_CAMERA_MAX_NUM_SENSORS];
    bool mFlashOn[MM_CAMERA_MAX_NUM_SENSORS];
    bool mCameraOpen[MM_CAMERA_MAX_NUM_SENSORS];
};

} // namespace qcamera

#endif // __QCAMERA_FLASH_H__
