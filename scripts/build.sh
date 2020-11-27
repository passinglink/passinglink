#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

. "$SCRIPT_PATH/boards.sh"

set -euxo pipefail

mkdir -p "$BUILD_DIR"

if [[ $PL_MCUBOOT_SUPPORTED == 1 ]]; then
  # Build MCUboot.
  OVERLAY_FILE="$(realpath "$ROOT/passinglink/boards/$BOARD.overlay")"
  OVERLAY=""
  [[ -f "$OVERLAY_FILE" ]] && OVERLAY="-DDTC_OVERLAY_FILE=$OVERLAY_FILE"

  west build -p -d "$BUILD_DIR/mcuboot" -s "$ROOT/bootloader/mcuboot/boot/zephyr" -- \
    -DCONFIG_SINGLE_APPLICATION_SLOT=y \
    -DCONFIG_SIZE_OPTIMIZATIONS=y \
    -DCONFIG_USB_DEVICE_STACK=y \
    -DCONFIG_USB_DEVICE_VID=1209 -DCONFIG_USB_DEVICE_PID=214d \
    -DCONFIG_USB_DEVICE_PRODUCT="\"Passing Link Bootloader\"" \
    $OVERLAY \
    ${PL_MCUBOOT_OPTS}
  cp "$BUILD_DIR/mcuboot/zephyr/zephyr.bin" "$BUILD_DIR/mcuboot.bin"
fi

if [[ $PL_MCUBOOT_SUPPORTED == 1 ]]; then
  # Build Passing Link with MCUboot support.
  west build -p -d "$BUILD_DIR/pl" -s "$ROOT/passinglink" -- \
    -DCONFIG_BOOTLOADER_MCUBOOT=y

  # Sign Passing Link.
  "$SCRIPT_PATH/sign.sh" "$BUILD_DIR/pl/zephyr/zephyr.bin" "$BUILD_DIR/pl.bin"
else
  # Build Passing Link without MCUboot support.
  west build -p -d "$BUILD_DIR/pl" -s "$ROOT/passinglink"

  # Don't bother signing the image.
  cp "$BUILD_DIR/pl/zephyr/zephyr.bin" "$BUILD_DIR/pl.bin"
fi
