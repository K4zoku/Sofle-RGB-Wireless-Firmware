#!/usr/bin/env bash
# Build ZMK firmware locally using the official ZMK build image via podman.
# Mirrors the build matrix in build.yaml (same commands GitHub Actions runs).
# The whole west/build flow runs inside ONE container so the Zephyr cmake
# package registry (zephyr-export) survives across steps.
#
# Usage:
#   ./build-local.sh              # build all targets
#   ./build-local.sh sofle_left   # build one target (sofle_left | sofle_right | settings_reset)
#
# Artifacts land in build/<shield>/zephyr/zmk.uf2
# The west workspace (.west/, zmk/, zephyr/, modules/) persists in this repo
# dir, so subsequent builds skip the big download.

set -euo pipefail

IMAGE="docker.io/zmkfirmware/zmk-build-arm:stable"
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET="${1:-all}"

exec podman run --rm \
  -v "${REPO_DIR}:/workspace" \
  -w /workspace \
  "${IMAGE}" \
  bash /workspace/build-inner.sh "${TARGET}"
