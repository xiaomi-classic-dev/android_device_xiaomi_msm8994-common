/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef XIAOMI_MSM8994_SHIM_IPOWERMANAGER_H
#define XIAOMI_MSM8994_SHIM_IPOWERMANAGER_H

#include <binder/IInterface.h>
#include <utils/Errors.h>

namespace android {

// Android 11 compatible C++ interface used by the legacy display blobs.
class IPowerManager : public IInterface {
public:
    enum {
        ACQUIRE_WAKE_LOCK = IBinder::FIRST_CALL_TRANSACTION,
        ACQUIRE_WAKE_LOCK_UID = IBinder::FIRST_CALL_TRANSACTION + 1,
        RELEASE_WAKE_LOCK = IBinder::FIRST_CALL_TRANSACTION + 2,
        UPDATE_WAKE_LOCK_UIDS = IBinder::FIRST_CALL_TRANSACTION + 3,
        POWER_HINT = IBinder::FIRST_CALL_TRANSACTION + 4,
        GO_TO_SLEEP = IBinder::FIRST_CALL_TRANSACTION + 9,
        REBOOT = IBinder::FIRST_CALL_TRANSACTION + 21,
        SHUTDOWN = IBinder::FIRST_CALL_TRANSACTION + 23,
        CRASH = IBinder::FIRST_CALL_TRANSACTION + 24,
    };

    DECLARE_META_INTERFACE(PowerManager)

    virtual status_t acquireWakeLock(int flags, const sp<IBinder>& lock, const String16& tag,
                                    const String16& packageName, bool isOneWay = false) = 0;
    virtual status_t acquireWakeLockWithUid(int flags, const sp<IBinder>& lock,
                                           const String16& tag, const String16& packageName,
                                           int uid, bool isOneWay = false) = 0;
    virtual status_t releaseWakeLock(const sp<IBinder>& lock, int flags,
                                     bool isOneWay = false) = 0;
    virtual status_t updateWakeLockUids(const sp<IBinder>& lock, int len, const int* uids,
                                        bool isOneWay = false) = 0;
    virtual status_t powerHint(int hintId, int data) = 0;
    virtual status_t goToSleep(int64_t eventTimeMs, int reason, int flags) = 0;
    virtual status_t reboot(bool confirm, const String16& reason, bool wait) = 0;
    virtual status_t shutdown(bool confirm, const String16& reason, bool wait) = 0;
    virtual status_t crash(const String16& message) = 0;
};

}  // namespace android

#endif  // XIAOMI_MSM8994_SHIM_IPOWERMANAGER_H
