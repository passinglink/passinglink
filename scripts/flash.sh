#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

. "$SCRIPT_PATH/boards.sh"

set -euxo pipefail

if [[ $PL_MCUBOOT_SUPPORTED == "1" ]]; then
  west flash -d "$BUILD_DIR/mcuboot"
  pyocd flash -e sector -a 0x10000 -t nrf52840 "$BUILD_DIR/pl.bin"
else
  west flash -d "$BUILD_DIR/pl"
fi
