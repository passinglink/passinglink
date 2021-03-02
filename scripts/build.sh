#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

. "$SCRIPT_PATH/boards.sh"

set -euxo pipefail

mkdir -p "$BUILD_DIR"

if [[ $PL_MCUBOOT_SUPPORTED == 1 && "${PL_SKIP_MCUBOOT-}" != 1 ]]; then
  # Build MCUboot.
  OVERLAY_FILE="$(realpath "$ROOT/passinglink/boards/$BOARD.overlay")"
  OVERLAY=""
  [[ -f "$OVERLAY_FILE" ]] && OVERLAY="-DDTC_OVERLAY_FILE=$OVERLAY_FILE"

  if [ ! -d "$BUILD_DIR/mcuboot" ]; then
    west build --cmake-only -d "$BUILD_DIR/mcuboot" -s "$ROOT/bootloader/mcuboot/boot/zephyr" -- \
      -DCONFIG_SINGLE_APPLICATION_SLOT=y \
      -DCONFIG_SIZE_OPTIMIZATIONS=y \
      -DCONFIG_USB_DEVICE_STACK=y \
      -DCONFIG_USB_DEVICE_BOS=y \
      -DCONFIG_USB_DEVICE_VID=0x1209 \
      -DCONFIG_USB_DEVICE_PID=0x214d \
      -DCONFIG_USB_DEVICE_DFU_PID=0x214d \
      -DCONFIG_USB_DEVICE_MANUFACTURER="\"Passing Link\"" \
      -DCONFIG_USB_DEVICE_PRODUCT="\"$BOARD\"" \
      -DCONFIG_USB_REQUEST_BUFFER_SIZE=4096 \
      $OVERLAY \
      ${PL_MCUBOOT_OPTS}
  fi

  west build -d "$BUILD_DIR/mcuboot"

  cp "$BUILD_DIR/mcuboot/zephyr/zephyr.hex" "$BUILD_DIR/mcuboot.hex"
fi

if [[ "${PL_SKIP_PL-}" != 1 ]]; then
  if [[ $PL_MCUBOOT_SUPPORTED == 1 ]]; then
    # Build Passing Link with MCUboot support.
    if [ ! -d "$BUILD_DIR/pl" ]; then
      west build --cmake-only -d "$BUILD_DIR/pl" -s "$ROOT/passinglink" -- \
        -DCONFIG_BOOTLOADER_MCUBOOT=y
    fi

    west build -d "$BUILD_DIR/pl"

    # Sign Passing Link.
    # Create both .bin and .hex files, for dfu-util and pyocd respectively.
    "$SCRIPT_PATH/sign.sh" "$BUILD_DIR/pl/zephyr/zephyr.bin" "$BUILD_DIR/pl.bin"
    "$SCRIPT_PATH/sign.sh" "$BUILD_DIR/pl/zephyr/zephyr.hex" "$BUILD_DIR/pl.hex"
  else
    # Build Passing Link without MCUboot support.
    west build -d "$BUILD_DIR/pl" -s "$ROOT/passinglink"

    # Don't bother signing the image.
    cp "$BUILD_DIR/pl/zephyr/zephyr.bin" "$BUILD_DIR/pl.bin"
    cp "$BUILD_DIR/pl/zephyr/zephyr.hex" "$BUILD_DIR/pl.hex"
  fi
fi
