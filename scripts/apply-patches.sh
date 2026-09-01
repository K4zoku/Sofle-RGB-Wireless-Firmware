#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

apply_patch() {
  local repo="$1"
  local patch="$2"

  if git -C "${ROOT}/${repo}" apply --check "${ROOT}/config/patches/${patch}"; then
    git -C "${ROOT}/${repo}" apply "${ROOT}/config/patches/${patch}"
  elif git -C "${ROOT}/${repo}" apply --reverse --check "${ROOT}/config/patches/${patch}"; then
    echo "Already applied: ${patch}"
  else
    echo "Cannot apply: ${patch}" >&2
    return 1
  fi
}

apply_patch zmk zmk-rgb-via.patch
apply_patch zmk zmk-encoder-persistence.patch
apply_patch zmk zmk-behavior-queue.patch
apply_patch zmk zmk-mouse-buttons.patch
apply_patch zmk zmk-via-mouse-fixed-speed.patch
apply_patch zmk-raw-hid zmk-raw-hid-via-descriptor.patch
