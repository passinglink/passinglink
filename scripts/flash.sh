#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

. "$SCRIPT_PATH/boards.sh"

set -euxo pipefail

if ! pyocd list -t | egrep -q "^\s+$PL_PYOCD_TYPE\s"; then
  echo "pyocd support missing for '$PL_PYOCD_TYPE': try \`pyocd pack --install $PL_PYOCD_TYPE\`?"
  exit 1
fi

pyocd flash -e sector -t $PL_PYOCD_TYPE "$BUILD_DIR/mcuboot.hex"
pyocd flash -e sector -t $PL_PYOCD_TYPE "$BUILD_DIR/pl.hex"
