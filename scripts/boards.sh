#!/bin/bash

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  echo "This script shouldn't be used directly."
  exit 1
fi

if [ -z "$BOARD" ]; then
  echo \$BOARD must be set.
  exit 1
fi

if [[ "$BOARD" == "pl_e73" ]]; then
  PL_MCUBOOT_SUPPORTED=1
  PL_MCUBOOT_OPTS="
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIO_0\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=0
  "
elif [[ "$BOARD" == "pl_dongle" ]]; then
  PL_MCUBOOT_SUPPORTED=1
  PL_MCUBOOT_OPTS="
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIO_0\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=18
  "
else
  PL_MCUBOOT_SUPPORTED=0
fi

if [[ "$PL_MCUBOOT_SUPPORTED" == "1" ]]; then
  echo "MCUboot supported on board '$BOARD'"
else
  echo "MCUboot not supported on board '$BOARD'"
fi
