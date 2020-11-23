#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

. "$SCRIPT_PATH/boards.sh"

set -euxo pipefail

if [[ $PL_MCUBOOT_SUPPORTED == "1" ]] || [[ -n $PL_PROVISIONING_OFFSET ]]; then
  pyocd flash -e sector -a 0x0 -t $PL_PYOCD_TYPE "$BUILD_DIR/mcuboot.bin"
  pyocd flash -e sector -a 0x10000 -t $PL_PYOCD_TYPE "$BUILD_DIR/pl.bin"
else
  west flash -d "$BUILD_DIR/pl"
fi
