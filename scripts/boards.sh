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

unset PL_MCUBOOT_SUPPORTED PL_MCUBOOT_OPTS

# TODO: Figure out the provisioning partition offset/size from the dts directly.
if [[ "$BOARD" == "microdash" ]]; then
  PL_PYOCD_TYPE=nrf52840

  PL_MCUBOOT_SUPPORTED=1
  PL_MCUBOOT_OPTS="
    -DCONFIG_I2C=y
    -DCONFIG_PASSINGLINK_DISPLAY_SSD1306=y
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIO_0\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=13
  "
elif [[ "$BOARD" == "pl_e73" ]]; then
  PL_PYOCD_TYPE=nrf52840

  PL_MCUBOOT_SUPPORTED=1
  PL_MCUBOOT_OPTS="
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIO_0\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=0
  "
elif [[ "$BOARD" == "pl_stm32" ]]; then
  PL_PYOCD_TYPE=stm32_f103re

  PL_MCUBOOT_SUPPORTED=0
# PL_MCUBOOT_OPTS="
#   -DCONFIG_BOOT_USB_DFU_GPIO=y
#   -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIOC\"
#   -DCONFIG_BOOT_USB_DFU_DETECT_PIN=15
# "
elif [[ "$BOARD" == "pl_dongle" ]]; then
  PL_PYOCD_TYPE=nrf52840

  PL_MCUBOOT_SUPPORTED=1
  PL_MCUBOOT_OPTS="
    -DCONFIG_I2C=y
    -DCONFIG_PASSINGLINK_DISPLAY_SSD1306=y
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIO_0\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=18
  "
elif [[ "$BOARD" == "particle_xenon" ]]; then
  PL_PYOCD_TYPE=nrf52840

  PL_MCUBOOT_SUPPORTED=1
  PL_MCUBOOT_OPTS="
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIO_0\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=11
  "
elif [[ "$BOARD" == "pl_bluepill" ]]; then
  PL_PYOCD_TYPE=stm32f103c8

  PL_MCUBOOT_SUPPORTED=0
elif [[ "$BOARD" == "stm32f4_disco" ]]; then
  PL_PYOCD_TYPE=stm32f407vg

  PL_MCUBOOT_SUPPORTED=1
  # Use GPIO E7 ("Up key") here because the "User" button needs internal
  # pull-down, which is not yet supported on MCUBoot (and I'm too lazy to add
  # it).
  PL_MCUBOOT_OPTS="
    -DCONFIG_LOG=n
    -DCONFIG_BOOT_USB_DFU_GPIO=y
    -DCONFIG_BOOT_USB_DFU_DETECT_PORT=\"GPIOE\"
    -DCONFIG_BOOT_USB_DFU_DETECT_PIN=7
  "
else
  echo "warning: unsupported board: \"$BOARD\" (edit scripts/boards.sh?)"
  PL_MCUBOOT_SUPPORTED=0
fi

if [[ "$PL_MCUBOOT_SUPPORTED" == "1" ]]; then
  echo "MCUboot supported on board '$BOARD'"
else
  echo "MCUboot not supported on board '$BOARD'"
fi
