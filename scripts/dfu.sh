#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

. "$SCRIPT_PATH/boards.sh"

if [ "$PL_MCUBOOT_SUPPORTED" != "1" ]; then
  echo "Board '$BOARD' doesn't support MCUboot"
  exit 1
fi

set -euxo pipefail

PL_SKIP_MCUBOOT=1 PL_SKIP_PL=0 "$SCRIPT_PATH/build.sh"

"$SCRIPT_PATH/sign.sh" "$BUILD_DIR/pl/zephyr/zephyr.bin" "$BUILD_DIR/pl.bin"

dfu-util --alt 0 --download "$BUILD_DIR/pl.bin"
