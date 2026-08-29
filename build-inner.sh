#!/usr/bin/env bash
# Runs INSIDE the zmk-build-arm container (invoked by build-local.sh).
# /workspace is the mounted config repo.

set -euo pipefail

TARGET="${1:-all}"
BOARD="nice_nano_v2"
CONFIG=/workspace/config

# 1) Init the west workspace once (.west persists in the mounted repo dir).
if [ ! -d /workspace/.west ]; then
  echo "==> west init -l ${CONFIG}"
  west init -l "${CONFIG}"
fi

echo "==> west update"

west update
ZMK_REV="edf5c0814fd3ea202e43aad2d68fd32e882a518c"
git -C /workspace/zmk reset --hard "${ZMK_REV}" >/dev/null
ZMK_RGB_PATCH="${CONFIG}/patches/zmk-rgb-via.patch"
git -C /workspace/zmk apply "${ZMK_RGB_PATCH}"
ZMK_ENCODER_PATCH="${CONFIG}/patches/zmk-encoder-persistence.patch"
git -C /workspace/zmk apply "${ZMK_ENCODER_PATCH}"
ZMK_QUEUE_PATCH="${CONFIG}/patches/zmk-behavior-queue.patch"
git -C /workspace/zmk apply "${ZMK_QUEUE_PATCH}"
ZMK_MOUSE_PATCH="${CONFIG}/patches/zmk-mouse-buttons.patch"
git -C /workspace/zmk apply "${ZMK_MOUSE_PATCH}"
ZMK_VIA_MOUSE_PATCH="${CONFIG}/patches/zmk-via-mouse-fixed-speed.patch"
git -C /workspace/zmk apply "${ZMK_VIA_MOUSE_PATCH}"
RAW_HID_REV="6a37765dfab6197292e7a9f47305dcf87386d56a"
RAW_HID_PATCH="${CONFIG}/patches/zmk-raw-hid-via-descriptor.patch"
git -C /workspace/zmk-raw-hid reset --hard "${RAW_HID_REV}" >/dev/null
git -C /workspace/zmk-raw-hid apply "${RAW_HID_PATCH}"
echo "==> west zephyr-export"
west zephyr-export

build_left() {
  # Central half: ZMK Studio over USB (snippet) + Studio Kconfig flags.
  west build -p always -s /workspace/zmk/app -b "${BOARD}" -d /workspace/build/sofle_left \
    -S studio-rpc-usb-uart -- \
    -DSHIELD="sofle_left raw_hid_adapter" -DZMK_CONFIG="${CONFIG}" \
    -DCONFIG_ZMK_STUDIO=y -DCONFIG_ZMK_STUDIO_LOCKING=n \
    -DCONFIG_ZMK_STUDIO_LOCK_ON_DISCONNECT=n
}

build_right() {
  west build -s /workspace/zmk/app -b "${BOARD}" -d /workspace/build/sofle_right -- \
    -DSHIELD=sofle_right -DZMK_CONFIG="${CONFIG}"
}

build_settings_reset() {
  west build -s /workspace/zmk/app -b "${BOARD}" -d /workspace/build/settings_reset -- \
    -DSHIELD=settings_reset -DZMK_CONFIG="${CONFIG}"
}
build_pipar_flake() {
  west build -p always -s /workspace/zmk/app -b "${BOARD}" -d /workspace/build/pipar_flake -- \
    -DSHIELD="pipar_flake raw_hid_adapter" -DZMK_CONFIG="${CONFIG}"
}


build_all() {
  build_left
  build_right
  build_settings_reset
  build_pipar_flake
}
case "${TARGET}" in
  all)            build_all ;;
  sofle_left)     build_left ;;
  sofle_right)    build_right ;;
  settings_reset) build_settings_reset ;;
  pipar_flake)    build_pipar_flake ;;
  *)
    echo "Unknown target: ${TARGET}" >&2
    echo "Usage: build-local.sh [all|sofle_left|sofle_right|settings_reset|pipar_flake]" >&2
    exit 1
    ;;
esac

echo
echo "Done. UF2 files:"
for d in /workspace/build/*/zephyr/zmk.uf2; do [ -f "$d" ] && echo "  $d"; done
