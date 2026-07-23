#
# Copyright 2025 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Conditionally configure the B300 support
########################################################################
# Conditionally configure the B300 support on Linux and Windows
if(ENABLE_B300 AND (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR WIN32))
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_bar0_spi_core.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_image_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_jesd_core.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_mb_controller.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_mb_eeprom.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_mb_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_pcie_mgr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_pcie_link.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_radio_control.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_clock_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_gps_control.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b300_experts.cpp
    )

    # Add platform-specific session and fifo implementations
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Add all CPP files from inchworm/user/libnifpga-usrp/src/
        file(GLOB B300_INCHWORM_LINUX_SRCS
            ${CMAKE_CURRENT_SOURCE_DIR}/inchworm/user/libnifpga-usrp/src/*.cpp
        )
        # Remove Windows-specific files from the list
        list(REMOVE_ITEM B300_INCHWORM_LINUX_SRCS
            ${CMAKE_CURRENT_SOURCE_DIR}/inchworm/user/libnifpga-usrp/src/DeviceFile_win32.cpp
        )
        LIBUHD_APPEND_SOURCES(${B300_INCHWORM_LINUX_SRCS})
        LIBUHD_APPEND_SOURCES(
            ${CMAKE_CURRENT_SOURCE_DIR}/b300_pcie_fifo_linux.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/b300_pcie_session_linux.cpp
        )
        message(STATUS "B300 support enabled for Linux")
    elseif(WIN32)
        # Add platform-specific inchworm source files for Windows
        set(B300_INCHWORM_WIN32_SRCS
            ${CMAKE_CURRENT_SOURCE_DIR}/inchworm/user/libnifpga-usrp/src/DeviceFile_win32.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/inchworm/user/libnifpga-usrp/src/ErrnoMap.cpp
        )
        LIBUHD_APPEND_SOURCES(${B300_INCHWORM_WIN32_SRCS})
        LIBUHD_APPEND_SOURCES(
            ${CMAKE_CURRENT_SOURCE_DIR}/b300_pcie_fifo_windows.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/b300_pcie_session_windows.cpp
        )
        # Link Windows-specific libraries required for device enumeration and management
        LIBUHD_APPEND_LIBS(setupapi cfgmgr32)
        message(STATUS "B300 support enabled for Windows")
    else()
        message(FATAL_ERROR "B300 support is only available on Linux and Windows. Current system: ${CMAKE_SYSTEM_NAME}")
    endif()
elseif(ENABLE_B300)
    message(STATUS "B300 support is only available on Linux and Windows. Skipping B300 on ${CMAKE_SYSTEM_NAME}.")
endif()
