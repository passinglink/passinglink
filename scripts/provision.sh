#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

. "$SCRIPT_PATH/boards.sh"

if [ $# != 4 ]; then
  echo usage: $0 BOARD_NAME PRIVATE_KEY_DER SERIAL SIGNATURE
  exit 1
fi

set -euxo pipefail

DTS_HEADER="$BUILD_DIR/pl/zephyr/include/generated/devicetree_legacy_unfixed.h"
if [[ ! -f "$DTS_HEADER" ]]; then
  echo "Building $BOARD to generate DTS header..."
  $SCRIPT_PATH/build.sh
fi

OFFSET_PARTS=( $(grep "#define DT_FLASH_AREA_PROVISIONING_OFFSET_0" $DTS_HEADER) )
SIZE_PARTS=( $(grep "#define DT_FLASH_AREA_PROVISIONING_SIZE_0" $DTS_HEADER) )
OFFSET=${OFFSET_PARTS[2]}
SIZE=${SIZE_PARTS[2]}

if ! pyocd list -t | egrep -q "^\s+$PL_PYOCD_TYPE\s"; then
  echo "pyocd support missing for '$PL_PYOCD_TYPE': try \`pyocd pack --install $PL_PYOCD_TYPE\`?"
  exit 1
fi

"$ROOT/tools/mbed_embed/mbed_embed" $OFFSET $SIZE "$1" "$2" "$3" "$4" > \
  "$BUILD_DIR/provisioning.bin"
pyocd flash -e sector -a $OFFSET -t $PL_PYOCD_TYPE $BUILD_DIR/provisioning.bin
