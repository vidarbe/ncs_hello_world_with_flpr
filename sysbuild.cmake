# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(DEFINED SB_CONFIG_REMOTE_BOARD)
  ExternalZephyrProject_Add(
    APPLICATION coprocessor
    SOURCE_DIR ${APP_DIR}/coprocessor
    BOARD ${SB_CONFIG_REMOTE_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )
  
  add_dependencies(${DEFAULT_IMAGE} coprocessor)
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} coprocessor)
endif()
