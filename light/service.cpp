// SPDX-License-Identifier: Apache-2.0
#include "Light.h"
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>
#include <string>

int main() {
    using aidl::android::hardware::light::ILights;
    using aidl::android::hardware::light::implementation::Light;
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    auto service = ndk::SharedRefBase::make<Light>();
    const std::string instance = std::string(ILights::descriptor) + "/default";
    if (AServiceManager_addService(service->asBinder().get(), instance.c_str()) != STATUS_OK) {
        ALOGE("Cannot register Light HAL service");
        return 1;
    }
    ABinderProcess_joinThreadPool();
    return 1;
}
