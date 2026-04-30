# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world_with_flpr)

set(FLPR_BIN "${CMAKE_BINARY_DIR}/../coprocessor/zephyr/zephyr.bin")
set(FLPR_HDR "${CMAKE_CURRENT_BINARY_DIR}/include/flpr_firmware.h")
set(BIN2HDR  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/bin_to_header.py")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")

add_custom_command(
    OUTPUT  "${FLPR_HDR}"
    COMMAND "${PYTHON_EXECUTABLE}" -X utf8
                "${BIN2HDR}"
                --input      "${FLPR_BIN}"
                --output     "${FLPR_HDR}"
                --array-name flpr_firmware
    DEPENDS "${FLPR_BIN}" "${BIN2HDR}"
    COMMENT "Generating FLPR firmware header from zephyr.bin"
    VERBATIM
)

add_custom_target(flpr_header_gen DEPENDS "${FLPR_HDR}")
add_dependencies(app flpr_header_gen)
target_include_directories(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/include")
target_sources(app PRIVATE 
    src/main.c
    src/ble_dfu.c
)
