
## Passing Link
[![CircleCI](https://circleci.com/gh/passinglink/passinglink.svg?style=svg)](https://circleci.com/gh/passinglink/passinglink)

Passing Link is an open source game controller firmware implementation focused on minimizing latency and supporting inexpensive development boards. Currently, the latency as measured by [WydD](https://twitter.com/wydd)'s [usblag](https://gitlab.com/loic.petit/usblag) project is around 0.95ms.

### Features
- USB output
  - PS3 output
  - PS4 output, with controller authentication supported with [extracted keys](https://fail0verflow.com/blog/2018/ps4-ds4/)
  - Nintendo Switch output (thanks to [progmem](https://github.com/progmem) for [researching Switch controllers](https://github.com/progmem/Switch-Fightstick))
  - PC output via PS4
  - Console autodetection (detects Switch and PS3, with fallback to PS4)
- Razer Panthera touchpad support
- USB firmware upgrade support

### Future Goals
- Unimplemented hardware support
  - PS4 audio output
- Act as a USB decoder/converter
  - Input/output over SPI
  - USB input

### Compiling
```
# Install tool dependencies
apt-get -y install --no-install-recommends build-essential cmake git ninja-build python3-pip python3-setuptools python3-pyelftools wget

# Install the Zephyr SDK
ZEPHYR_SDK_VERSION=0.11.1
export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}-setup.run
sh zephyr-sdk-${ZEPHYR_SDK_VERSION}-setup.run -- -d $ZEPHYR_SDK_INSTALL_DIR

# Install west, Zephyr's source management tool
pip3 install west

# Checkout the source
west init -m https://github.com/passinglink/passinglink passinglink
cd passinglink
west update

# Build and flash
cd passinglink
west build -b pl_bluepill -t menuconfig
west flash

# See also: https://github.com/passinglink/zephyr-docker/blob/master/Dockerfile
```

### Supported hardware
Passing Link is based on the widely supported Zephyr RTOS with no specific hardware requirements, so it should be portable to a wide variety of microcontrollers. The following is a list of microcontrollers/development boards that are actively used for development:

- [Custom PCB with NRF52840](https://github.com/passinglink/pcb) (aka `pl_e73`)
  - 64MHz, 256kB RAM, 1MB flash
  - Primary development target
  - Bluetooth LE support (but not useful for implementing a wireless controller, because of missing Bluetooth Classic support)
- Various nRF development boards
  - 64MHz, 256kB RAM, 1MB flash
  - [nRF52840 MDK USB Dongle](https://wiki.makerdiary.com/nrf52840-mdk-usb-dongle/) (aka `pl_dongle`)
    - $18 on Amazon
    - limited GPIOs
  - [Adafruit Feather nRF52840 Express](https://www.adafruit.com/product/4062)
    - $25
    - 21 GPIOs
  - Particle Xenon (discontinued by manufacturer)
- Generic STM32F103 Bluepill boards (aka `pl_bluepill`)
  - 72MHz, 20kB RAM, 64kB flash
  - Recommended against due to resource constraints, but will be supported for as long as is feasible
  - [$3 from RobotDyn](https://robotdyn.com/stm32f103-stm32-arm-mini-system-dev-board-stm-firmware.html)
- STM32F4 Discovery board (STM32F407VG)
  - 168MHz, 128kB RAM + 64kB CCM, 1MB flash
  - 2 USB controllers (currently only 1 can be used).
  - [~$22 from Digi-Key](https://www.digikey.com/en/products/detail/stmicroelectronics/STM32F407G-DISC1/5824404). Cheaper option should be available elsewhere.

### Copying
Passing Link is open source software licensed under the MIT license.

Passing Link depends on and makes modifications to Zephyr and mbedTLS, both of which are licensed under the Apache-2.0 license. The modified sources can be found in the [GitHub organization](https://github.com/passinglink).
