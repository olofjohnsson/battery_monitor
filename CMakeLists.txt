#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(promicro_battery_monitor)

set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/boards)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/application/application.c
  src/bluetooth/bluetooth.c
  src/bluetooth/service.c
  src/sensor/main_voltage.c
  src/sensor/internal_temp.c
  src/hardware/led.c
  src/hardware/mux.c
)

# NORDIC SDK APP END
zephyr_library_include_directories(.)

# add_custom_command(TARGET app POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/zephyr/app_update.bin C:/Users/Olof/OneDrive/DFU
#     COMMENT "Copying app_update.bin to specified folder"
# )
