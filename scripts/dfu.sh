#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

if [ -z "$BOARD" ]; then
  echo \$BOARD must be set.
  exit 1
fi

set -euxo pipefail

BUILD_DIR="build_$BOARD"

## Build, sign, and flash Passing Link.
if [ ! -d "$BUILD_DIR/pl" ]; then
  west build --cmake-only -s "$ROOT/passinglink" -- \
    -DCONFIG_BOOTLOADER_MCUBOOT=y
fi

west build -d "$BUILD_DIR/pl"
"$SCRIPT_PATH/sign.sh" "$BUILD_DIR/pl/zephyr/zephyr.bin" "$BUILD_DIR/pl.bin"

dfu-util --alt 0 --download "$BUILD_DIR/pl.bin"
