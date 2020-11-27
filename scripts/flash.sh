#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT="$SCRIPT_PATH/../.."

. "$SCRIPT_PATH/boards.sh"

set -euxo pipefail

if [[ $PL_MCUBOOT_SUPPORTED == "1" ]] || [[ -n $PL_PROVISIONING_OFFSET ]]; then
  if ! pyocd list -t | egrep -q "^\s+$PL_PYOCD_TYPE\s"; then
    echo "pyocd support missing for '$PL_PYOCD_TYPE': try \`pyocd pack --install $PL_PYOCD_TYPE\`?"
    exit 1
  fi

  pyocd flash -e sector -t $PL_PYOCD_TYPE "$BUILD_DIR/mcuboot.hex"
  pyocd flash -e sector -t $PL_PYOCD_TYPE "$BUILD_DIR/pl.hex"
else
  west flash -d "$BUILD_DIR/pl"
fi
