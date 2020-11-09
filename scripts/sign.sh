#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")
ROOT=$SCRIPT_PATH/../..

if [ $# != 2 ]; then
  echo usage: $0 INPUT OUTPUT
  exit 1
fi

set -euxo pipefail

$ROOT/bootloader/mcuboot/scripts/imgtool.py sign \
  --key $ROOT/bootloader/mcuboot/root-rsa-2048.pem \
  --header-size 0x200 \
  --align 8 \
  --version 1.2 \
  --slot-size 0x60000 \
  "$1" \
  "$2"
