/*
 * Copyright (C) 2017-2018 The LineageOS Project
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

#ifndef ANDROID_HARDWARE_LIGHT_V2_0_LIGHT_H
#define ANDROID_HARDWARE_LIGHT_V2_0_LIGHT_H

#include <aidl/android/hardware/light/BnLights.h>
#include <hardware/lights.h>
#include <map>
#include <mutex>
#include <vector>

using aidl::android::hardware::light::HwLight;
using LightState = aidl::android::hardware::light::HwLightState;
using Type = aidl::android::hardware::light::LightType;
using Flash = aidl::android::hardware::light::FlashMode;

typedef void (*LightStateHandler)(const LightState&);

struct LightBackend {
    Type type;
    LightState state;
    LightStateHandler handler;

    LightBackend(Type type, LightStateHandler handler) : type(type), handler(handler) {
        this->state.color = 0xff000000;
    }
};

namespace aidl::android::hardware::light::implementation {

class Light : public BnLights {
   public:
    ndk::ScopedAStatus setLightState(int32_t id, const HwLightState& state) override;
    ndk::ScopedAStatus getLights(std::vector<HwLight>* lights) override;

   private:
    std::mutex globalLock;
};

}  // namespace aidl::android::hardware::light::implementation

#endif  // ANDROID_HARDWARE_LIGHT_V2_0_LIGHT_H
