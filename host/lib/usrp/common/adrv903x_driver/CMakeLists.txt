#
# Copyright 2025 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/devices/adrv903x/public/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/devices/adrv903x/private/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/common")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/common/adi_error")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/common/adi_hal")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/common/adi_logging")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/platforms")
file(GLOB Adrv903xPublicSourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/devices/adrv903x/public/src/*.c)
file(GLOB Adrv903xPrivateSourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/devices/adrv903x/private/src/*.c)
file(GLOB Adrv903xPrivateBfSourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/devices/adrv903x/private/bf/*.c)

LIBUHD_APPEND_SOURCES(
    ${Adrv903xPublicSourceFiles}
    ${Adrv903xPrivateSourceFiles}
    ${Adrv903xPrivateBfSourceFiles}
    ${CMAKE_CURRENT_SOURCE_DIR}/common/adi_error/adi_common_error.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/adi_hal/adi_common_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/adi_logging/adi_common_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/adi_library.c
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/adi_platform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/common/tls.c
)

set(Adrv903xImageFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/images/ADRV9032_FW.bin
    ${CMAKE_CURRENT_SOURCE_DIR}/images/ADRV9032_PROFILE_122_88.bin
    ${CMAKE_CURRENT_SOURCE_DIR}/images/ADRV9032_PROFILE_125.bin
    ${CMAKE_CURRENT_SOURCE_DIR}/images/ADRV9032_RX_GAIN_TABLE.csv
    ${CMAKE_CURRENT_SOURCE_DIR}/images/ADRV9032_STREAM_122_88.bin
    ${CMAKE_CURRENT_SOURCE_DIR}/images/ADRV9032_STREAM_125.bin
)

UHD_INSTALL(FILES ${Adrv903xImageFiles} DESTINATION ${PKG_DATA_DIR}/images COMPONENT images)
