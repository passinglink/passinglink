#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

if [ -z "$BOARD" ]; then
  echo \$BOARD must be set.
  exit 1
fi

set -euxo pipefail

BUILD_DIR="build_$BOARD"
west flash -d "$BUILD_DIR/mcuboot"
pyocd flash -e sector -a 0x10000 -t nrf52840 "$BUILD_DIR/pl.bin"
