
## Passing Link
[![CircleCI](https://circleci.com/gh/passinglink/passinglink.svg?style=svg)](https://circleci.com/gh/passinglink/passinglink)

Passing Link is an open source game controller firmware implementation focused on minimizing latency and supporting inexpensive development boards. Currently, the latency as measured by [WydD](https://twitter.com/wydd)'s [usblag](https://gitlab.com/loic.petit/usblag) project is around 1.2ms, with available low hanging fruit that should be able to cut that by ~0.2 ms.

### Features
- USB output
  - PS4 output, with controller authentication supported with [extracted keys](https://fail0verflow.com/blog/2018/ps4-ds4/)
  - Nintendo Switch output (thanks to [progmem](https://github.com/progmem) for [researching Switch controllers](https://github.com/progmem/Switch-Fightstick)
  - PC output via PS4
  - Console autodetection (detects Switch, and falls back to PS4)

### Future Goals
- Unimplemented hardware support
  - SPI touchpad input
  - PS4 audio output
- Firmware update via USB DFU
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
west build -b pl_bluepill && west flash

# See also: https://github.com/passinglink/zephyr-docker/blob/master/Dockerfile
```


### Supported hardware
Passing Link is based on the widely supported Zephyr RTOS with no specific hardware requirements, so it should be portable to a wide variety of microcontrollers. The following is a list of microcontrollers/development boards that are actively used for development:

- Custom PCB with STM32F103RE
	- 72MHz, 64kB RAM, 512kB flash
 	- Primary development target: the raw microcontroller can be acquired from AliExpress for under $2 each shipped
 - RobotDyn STM32F303 Black Pill
	- 72 MHz, 40kB RAM, 256 kB flash
	- [$4.59 from RobotDyn](https://robotdyn.com/stm32f303cct6-256-kb-flash-stm32-arm-cortexr-m4-mini-system-dev-board-3326a9dd-3c19-11e9-910a-901b0ebb3621.html)
- Particle Xenon (nRF52840)
	- 64MHz, 256kB RAM, 1MB flash
	- Bluetooth LE support (but not useful for implementing a wireless controller, because of missing Bluetooth Classic support)
	- [$11 from Particle](https://store.particle.io/products/xenon), but discontinued
- Generic STM32F103 Bluepill boards
	- 72MHz, 20kB RAM, 64kB flash
	- Recommended against due to resource constraints, but will be supported for as long as is feasible
	- [$3 from RobotDyn](https://robotdyn.com/stm32f103-stm32-arm-mini-system-dev-board-stm-firmware.html)

### Copying
Passing Link is open source software licensed under the MIT license.

Passing Link depends on and makes modifications to Zephyr and mbedTLS, both of which are licensed under the Apache-2.0 license. The modified sources can be found in the [GitHub organization](https://github.com/passinglink).
