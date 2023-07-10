#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Conditionally configure the kintex7sdr support
########################################################################
if (ENABLE_KINTEX7SDR)
    LIBUHD_APPEND_SOURCES(
            ${CMAKE_CURRENT_SOURCE_DIR}/../usrp2/usrp2_fifo_ctrl.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/io_impl.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mb_eeprom.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/kintex7sdr_impl.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/clock_ctrl.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/codec_ctrl.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/dboard_iface.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/kintex7sdr_iface.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/kintex7sdr_image_loader.cpp
    )
endif (ENABLE_KINTEX7SDR)
