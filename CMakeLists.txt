# SPDX-License-Identifier: Apache-2.0

get_filename_component(MBEDTLS_CONFIG_ABSOLUTE "mbedtls-config.h" ABSOLUTE)

cmake_minimum_required(VERSION 3.13.1)

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(passinglink)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color -Wall -Wextra -Wno-unused")
list(APPEND app_sources
    src/arch.cpp
    src/main.cpp
    src/malloc.cpp
    src/input/input.cpp
    src/output/output.cpp
    src/output/usb/hid.cpp
    src/output/usb/usb.cpp
    src/output/usb/nx/hid.cpp
    src/output/usb/ps4/hid.cpp
    src/output/usb/ps4/auth.cpp
)
include_directories(
    src
    keys
    keys/default
)
target_sources(app PRIVATE ${app_sources})
set_property(TARGET app PROPERTY CXX_STANDARD 17)
