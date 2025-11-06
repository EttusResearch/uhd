#
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Find DPDK by using pkg-config. This sets various variables prefixed
# with "DPDK_".
#
# The following variables are used by the UHD build system:
#  DPDK_INCLUDE_DIRS, the include directories
#  DPDK_CFLAGS, the compiler flags
#  DPDK_LDFLAGS, the linker flags
#  DPDK_LIBRARIES, the list of libraries
#  DPDK_FOUND, If false, do not try to use DPDK.

include(FindPackageHandleStandardArgs)

find_package(PkgConfig)
PKG_CHECK_MODULES(DPDK libdpdk>=18.11)

find_package_handle_standard_args(
    DPDK
    DEFAULT_MSG
    DPDK_INCLUDE_DIRS
    DPDK_CFLAGS DPDK_LDFLAGS
    DPDK_LIBRARIES
)
mark_as_advanced(DPDK_INCLUDE_DIRS DPDK_CFLAGS DPDK_LDFLAGS DPDK_LIBRARIES)
