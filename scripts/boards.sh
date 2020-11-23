#!/bin/bash

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  echo "This script shouldn't be used directly."
  exit 1
fi

if [ -z "$BOARD" ]; then
  echo \$BOARD must be set.
  exit 1
fi

BUILD_DIR="build/$BOARD"

unset PL_MCUBOOT_SUPPORTED PL_MCUBOOT_OPTS PL_PROVISIONING_OFFSET PL_PROVISIONING_SIZE

# TODO: Figure out the provisioning partition offset/size from the dts directly.
if [[ "$BOARD" == "pl_e73" ]]; then
  PL_PROVISIONING_OFFSET=0xf8000
  PL_PROVISIONING_SIZE=0x8000
  PL_PYOCD_TYPE=nrf52840

  PL_MCUBOOT_SUPPORTED=1
  PL_MCUBOOT_OPTS="
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIO_0\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=0
  "
elif [[ "$BOARD" == "pl_dongle" ]]; then
  PL_PROVISIONING_OFFSET=0xf8000
  PL_PROVISIONING_SIZE=0x8000
  PL_PYOCD_TYPE=nrf52840

  PL_MCUBOOT_SUPPORTED=1
  PL_MCUBOOT_OPTS="
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIO_0\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=18
  "
elif [[ "$BOARD" == "particle_xenon" ]]; then
  PL_PROVISIONING_OFFSET=0xf8000
  PL_PROVISIONING_SIZE=0x8000
  PL_PYOCD_TYPE=nrf52840

  PL_MCUBOOT_SUPPORTED=1
  PL_MCUBOOT_OPTS="
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIO_0\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=11
  "
elif [[ "$BOARD" == "pl_bluepill" ]]; then
  PL_PROVISIONING_OFFSET=0x0800f000
  PL_PROVISIONING_SIZE=0x1000
  PL_PYOCD_TYPE=stm32f103rc

  PL_MCUBOOT_SUPPORTED=0
elif [[ "$BOARD" == "stm32f4_disco" ]]; then
  PL_PROVISIONING_OFFSET=0x0800f000
  PL_PROVISIONING_SIZE=0x8000
  PL_PYOCD_TYPE=stm32f407vg

  PL_MCUBOOT_SUPPORTED=0
else
  echo "warning: unsupported board: \"$BOARD\" (edit scripts/boards.sh?)"
  PL_MCUBOOT_SUPPORTED=0
fi

if [[ "$PL_MCUBOOT_SUPPORTED" == "1" ]]; then
  echo "MCUboot supported on board '$BOARD'"
else
  echo "MCUboot not supported on board '$BOARD'"
fi

if ! pyocd list -t | egrep -q "^\s+$PL_PYOCD_TYPE\s"; then
  echo "pyocd support missing for '$PL_PYOCD_TYPE': try \`pyocd pack --install $PL_PYOCD_TYPE\`?"
  exit 1
fi
