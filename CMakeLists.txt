# SPDX-License-Identifier: Apache-2.0

get_filename_component(MBEDTLS_CONFIG_ABSOLUTE "mbedtls-config.h" ABSOLUTE)

cmake_minimum_required(VERSION 3.13.1)

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(passinglink)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color -Wall -Wextra -Wno-unused")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/_version.cpp
    COMMAND ${CMAKE_COMMAND} -P
            ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
)

list(APPEND app_sources
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp

    src/arch.cpp
    src/bootloader.cpp
    src/main.cpp
    src/malloc.cpp
    src/provisioning.cpp
    src/shell.cpp
    src/bt/bt.cpp
    src/input/input.cpp
    src/input/queue.cpp
    src/output/led.cpp
    src/output/output.cpp
    src/output/usb/hid.cpp
    src/output/usb/usb.cpp
    src/output/usb/nx/hid.cpp
    src/output/usb/ps3/hid.cpp
    src/output/usb/ps4/hid.cpp
    src/output/usb/ps4/auth.cpp
    src/opt/gundam.cpp
)

target_sources_ifdef(CONFIG_PASSINGLINK_INPUT_TOUCHPAD_NONE app PRIVATE
    src/input/touchpad/none.cpp
)

target_sources_ifdef(CONFIG_PASSINGLINK_INPUT_TOUCHPAD_PANTHERA app PRIVATE
    src/input/touchpad/panthera.cpp
)

include_directories(
    src
    keys
)
target_sources(app PRIVATE ${app_sources})
set_property(TARGET app PROPERTY CXX_STANDARD 17)
