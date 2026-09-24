/*
 * Copyright (C) 2016 The CyanogenMod Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "xiaomi_readmac"

#include <cutils/log.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAC_ADDR_SIZE 6
#define WLAN_MAC_SOURCE "/persist/wlan_bt/wlan.mac"
#define WLAN_MAC_BIN "/data/misc/wifi/wlan_mac.bin"

int main(void) {
    uint8_t mac[MAC_ADDR_SIZE];
    uint8_t second[MAC_ADDR_SIZE];
    static const uint8_t zero[MAC_ADDR_SIZE] = {0};
    FILE *fp = fopen(WLAN_MAC_SOURCE, "rb");
    if (!fp) {
        ALOGE("Cannot open %s: %s", WLAN_MAC_SOURCE, strerror(errno));
        return 1;
    }
    size_t count = fread(mac, 1, sizeof(mac), fp);
    fclose(fp);
    if (count != sizeof(mac) || memcmp(mac, zero, sizeof(mac)) == 0) {
        ALOGE("Invalid WLAN MAC in %s", WLAN_MAC_SOURCE);
        return 1;
    }

    memcpy(second, mac, sizeof(second));
    for (int i = MAC_ADDR_SIZE - 1; i >= 0; --i) {
        if (++second[i] != 0) break;
    }

    fp = fopen(WLAN_MAC_BIN, "w");
    if (!fp) {
        ALOGE("Cannot open %s: %s", WLAN_MAC_BIN, strerror(errno));
        return 1;
    }
    int result = fprintf(fp,
                         "Intf0MacAddress=%02X%02X%02X%02X%02X%02X\n"
                         "Intf1MacAddress=%02X%02X%02X%02X%02X%02X\nEND\n",
                         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                         second[0], second[1], second[2], second[3], second[4], second[5]);
    if (fclose(fp) != 0 || result < 0) {
        ALOGE("Cannot write %s: %s", WLAN_MAC_BIN, strerror(errno));
        return 1;
    }
    return 0;
}
