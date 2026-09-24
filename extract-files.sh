#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2020 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

set -e

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

function blob_fixup() {
    case "${1}" in
        product/lib64/lib-imsvideocodec.so)
            if ! "${PATCHELF}" --print-needed "${2}" | grep -q '^libui_shim.so$'; then
                "${PATCHELF}" --add-needed libui_shim.so "${2}"
            fi
            ;;
        vendor/etc/init/ims_rtp_daemon.rc|vendor/etc/init/imsdatadaemon.rc|vendor/etc/init/imsqmidaemon.rc|vendor/etc/init/imsrcsd.rc)
            sed -i 's/vendor_qti_diag/diag/g' "${2}"
            if [[ "${1}" == "vendor/etc/init/ims_rtp_daemon.rc" || "${1}" == "vendor/etc/init/imsdatadaemon.rc" ]]; then
                sed -i '/^[[:space:]]*group radio diag inet log$/a\    capabilities NET_RAW' "${2}"
                sed -i '/^[[:space:]]*group radio wifi inet diag log$/a\    capabilities NET_RAW' "${2}"
            fi
            ;;
    esac
}

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

ONLY_COMMON=
ONLY_DEVICE_COMMON=
ONLY_TARGET=
KANG=
SECTION=

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        --only-common )
                ONLY_COMMON=true
                ;;
        --only-device-common )
                ONLY_DEVICE_COMMON=true
                ;;
        --only-target )
                ONLY_TARGET=true
                ;;
        -n | --no-cleanup )
                CLEAN_VENDOR=false
                ;;
        -k | --kang )
                KANG="--kang"
                ;;
        -s | --section )
                SECTION="${2}"; shift
                CLEAN_VENDOR=false
                ;;
        * )
                SRC="${1}"
                ;;
    esac
    shift
done

if [ -z "${SRC}" ]; then
    SRC="adb"
fi

if [ -z "${ONLY_TARGET}" ] && [ -z "${ONLY_DEVICE_COMMON}" ]; then
    # Initialize the helper for common device
    setup_vendor "${DEVICE_COMMON}" "${VENDOR}" "${ANDROID_ROOT}" true "${CLEAN_VENDOR}"

    extract "${MY_DIR}/proprietary-files.txt" "${SRC}" ${KANG} --section "${SECTION}"
fi

if [ -z "${ONLY_COMMON}" ] && [ -z "${ONLY_TARGET}" ] && [ -s "${MY_DIR}/../${DEVICE_SPECIFIED_COMMON}/proprietary-files.txt" ];then
    # Reinitialize the helper for device specified common
    source "${MY_DIR}/../${DEVICE_SPECIFIED_COMMON}/extract-files.sh"
    setup_vendor "${DEVICE_SPECIFIED_COMMON}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

    extract "${MY_DIR}/../${DEVICE_SPECIFIED_COMMON}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

if [ -z "${ONLY_COMMON}" ] && [ -z "${ONLY_DEVICE_COMMON}" ] && [ -s "${MY_DIR}/../${DEVICE}/proprietary-files.txt" ]; then
    # Reinitialize the helper for device
    source "${MY_DIR}/../${DEVICE}/extract-files.sh"
    setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

    extract "${MY_DIR}/../${DEVICE}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

"${MY_DIR}/setup-makefiles.sh"
