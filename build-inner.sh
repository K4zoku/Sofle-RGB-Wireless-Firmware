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
echo "==> west zephyr-export"
west zephyr-export

build_left() {
  # Central half: ZMK Studio over USB (snippet) + Studio Kconfig flags.
  west build -s /workspace/zmk/app -b "${BOARD}" -d /workspace/build/sofle_left \
    -S studio-rpc-usb-uart -- \
    -DSHIELD=sofle_left -DZMK_CONFIG="${CONFIG}" \
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

build_all() {
  build_left
  build_right
  build_settings_reset
}

case "${TARGET}" in
  all)            build_all ;;
  sofle_left)     build_left ;;
  sofle_right)    build_right ;;
  settings_reset) build_settings_reset ;;
  *)
    echo "Unknown target: ${TARGET}" >&2
    echo "Usage: build-local.sh [all|sofle_left|sofle_right|settings_reset]" >&2
    exit 1
    ;;
esac

echo
echo "Done. UF2 files:"
for d in /workspace/build/*/zephyr/zmk.uf2; do [ -f "$d" ] && echo "  $d"; done
