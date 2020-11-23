#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

. "$SCRIPT_PATH/boards.sh"

if [ $# != 4 ]; then
  echo usage: $0 BOARD_NAME PRIVATE_KEY_DER SERIAL SIGNATURE
  exit 1
fi

set -euxo pipefail

mkdir -p "$BUILD_DIR"

"$ROOT/tools/mbed_embed/mbed_embed" $PL_PROVISIONING_OFFSET $PL_PROVISIONING_SIZE "$1" "$2" "$3" "$4" > \
  "$BUILD_DIR/provisioning.bin"
pyocd flash -e sector -a $PL_PROVISIONING_OFFSET -t $PL_PYOCD_TYPE $BUILD_DIR/provisioning.bin
