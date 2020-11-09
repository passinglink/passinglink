#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

if [ -z "$BOARD" ]; then
  echo \$BOARD must be set.
  exit 1
fi

set -euxo pipefail

BUILD_DIR="build_$BOARD"
mkdir -p "$BUILD_DIR"

## Build and flash MCUboot.
west build -p -d "$BUILD_DIR/mcuboot" -s "$ROOT/bootloader/mcuboot/boot/zephyr"
west flash -d "$BUILD_DIR/mcuboot"

## Build, sign, and flash the DFU application.
west build -p -d "$BUILD_DIR/dfu" -s "$ROOT/zephyr/samples/subsys/usb/dfu" -- \
  -DCONFIG_USB_DEVICE_VID=1209 -DCONFIG_USB_DEVICE_PID=214d
"$SCRIPT_PATH/sign.sh" "$BUILD_DIR/dfu/zephyr/zephyr.bin" "$BUILD_DIR/dfu.bin"
pyocd flash -e sector -a 0x10000 -t nrf52840 "$BUILD_DIR/dfu.bin"

## Build, sign, and flash Passing Link.
west build -p -d "$BUILD_DIR/pl" -s "$ROOT/passinglink" -- -DCONFIG_BOOTLOADER_MCUBOOT=y
"$SCRIPT_PATH/sign.sh" "$BUILD_DIR/pl/zephyr/zephyr.bin" "$BUILD_DIR/pl.bin"

# We could flash directly with the following, but DFU should be faster.
#   pyocd flash -e sector -a 0x50000 -t nrf52840 "$BUILD_DIR/dfu.bin"
dfu-util --alt 1 --download "$BUILD_DIR/pl.bin"
