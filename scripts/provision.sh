#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

. "$SCRIPT_PATH/boards.sh"

if [ $# != 4 ]; then
  echo usage: $0 BOARD_NAME PRIVATE_KEY_DER SERIAL SIGNATURE
  exit 1
fi

set -euxo pipefail

DTS_HEADER="$BUILD_DIR/pl/zephyr/include/generated/devicetree_unfixed.h"
if [[ ! -f "$DTS_HEADER" ]]; then
  echo "Building $BOARD to generate DTS header..."
  $SCRIPT_PATH/build.sh
fi

HEADER="#include <devicetree.h>\n"
HEADER="$HEADER\n#define addr DT_REG_ADDR(DT_NODELABEL(provisioning_partition))"
HEADER="$HEADER\n#define size DT_REG_SIZE(DT_NODELABEL(provisioning_partition))"
HEADER="$HEADER\naddr size"

PARTS=($(
  echo -e "$HEADER" |
  cpp -I "$BUILD_DIR/pl/zephyr/include/generated" -I "$ROOT/zephyr/include" -w - |
  tail -n 1
))

OFFSET=${PARTS[0]}
SIZE=${PARTS[1]}

if ! pyocd list -t | egrep -q "^\s+$PL_PYOCD_TYPE\s"; then
  echo "pyocd support missing for '$PL_PYOCD_TYPE': try \`pyocd pack --install $PL_PYOCD_TYPE\`?"
  exit 1
fi

"$ROOT/tools/mbed_embed/mbed_embed" $OFFSET $SIZE "$1" "$2" "$3" "$4" > \
  "$BUILD_DIR/provisioning.bin"
pyocd flash -e sector -a $OFFSET -t $PL_PYOCD_TYPE $BUILD_DIR/provisioning.bin
