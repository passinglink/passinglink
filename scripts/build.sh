#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

if [ -z "$BOARD" ]; then
  echo \$BOARD must be set.
  exit 1
fi

if [ "$BOARD" == "pl_e73" ]; then
  MCUBOOT_OPTS="
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIO_0\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=0
  "
elif [ "$BOARD" == "pl_dongle" ]; then
  MCUBOOT_OPTS="
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIO_0\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=18
  "
else
  echo "Unsupported board: $BOARD"
  exit 1
fi

set -euxo pipefail

BUILD_DIR="build_$BOARD"
mkdir -p "$BUILD_DIR"

## Build MCUboot.
west build -p -d "$BUILD_DIR/mcuboot" -s "$ROOT/bootloader/mcuboot/boot/zephyr" -- \
  -DCONFIG_SINGLE_APPLICATION_SLOT=y \
  -DCONFIG_SIZE_OPTIMIZATIONS=y \
  -DCONFIG_USB_DEVICE_VID=1209 -DCONFIG_USB_DEVICE_PID=214d ${MCUBOOT_OPTS}
cp "$BUILD_DIR/mcuboot/zephyr/zephyr.bin" "$BUILD_DIR/mcuboot.bin"

## Build and sign Passing Link.
west build -p -d "$BUILD_DIR/pl" -s "$ROOT/passinglink" -- \
  -DCONFIG_BOOTLOADER_MCUBOOT=y
"$SCRIPT_PATH/sign.sh" "$BUILD_DIR/pl/zephyr/zephyr.bin" "$BUILD_DIR/pl.bin"
