/*
 * Copyright (C) 2016 The CyanogenMod Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "bdaddr_xiaomi"

#include <cutils/log.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAC_ADDR_SIZE 6
#define BD_ADDR_SOURCE "/persist/wlan_bt/bt.mac"
#define BD_ADDR_FILE "/data/misc/bluetooth/bdaddr"

int main(void) {
    uint8_t mac[MAC_ADDR_SIZE];
    static const uint8_t zero[MAC_ADDR_SIZE] = {0};
    FILE *fp = fopen(BD_ADDR_SOURCE, "rb");
    if (!fp) {
        ALOGE("Cannot open %s: %s", BD_ADDR_SOURCE, strerror(errno));
        return 1;
    }
    size_t count = fread(mac, 1, sizeof(mac), fp);
    fclose(fp);
    if (count != sizeof(mac) || memcmp(mac, zero, sizeof(mac)) == 0) {
        ALOGE("Invalid Bluetooth MAC in %s", BD_ADDR_SOURCE);
        return 1;
    }

    fp = fopen(BD_ADDR_FILE, "w");
    if (!fp) {
        ALOGE("Cannot open %s: %s", BD_ADDR_FILE, strerror(errno));
        return 1;
    }
    int result = fprintf(fp, "%02X:%02X:%02X:%02X:%02X:%02X\n",
                         mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    if (fclose(fp) != 0 || result < 0) {
        ALOGE("Cannot write %s: %s", BD_ADDR_FILE, strerror(errno));
        return 1;
    }
    return 0;
}
