#
# Copyright 2015 Ettus Research LLC
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# - Find libgps
# Find the Gpsd includes and client library
# This module defines
#  LIBGPS_INCLUDE_DIR, where to find gps.h
#  LIBGPS_LIBRARIES, the libraries needed by a GPSD client.
#  LIBGPS_FOUND, If false, do not try to use GPSD.
# also defined, but not for general use are
#  LIBGPS_LIBRARY, where to find the GPSD library.

include(FindPkgConfig)
PKG_CHECK_MODULES(PC_GPSD "libgps")
PKG_CHECK_MODULES(PC_GPSD_V3_11 "libgps >= 3.11")

if(PC_GPSD_FOUND AND NOT PC_GPSD_V3_11_FOUND)
    message(WARNING "GPSD version found is too old")
endif(PC_GPSD_FOUND AND NOT PC_GPSD_V3_11_FOUND)

if(PC_GPSD_V3_11_FOUND)
  find_path(
      LIBGPS_INCLUDE_DIR
      NAMES gps.h
      HINTS ${PC_GPSD_INCLUDE_DIR}
  )

  set(
      LIBGPS_NAMES
      ${LIBGPS_NAMES} gps
  )

  find_library(
      LIBGPS_LIBRARY
      NAMES ${LIBGPS_NAMES}
      HINTS ${PC_GPSD_LIBDIR}
  )
endif(PC_GPSD_V3_11_FOUND)

# handle the QUIETLY and REQUIRED arguments and set LIBGPS_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBGPS DEFAULT_MSG LIBGPS_LIBRARY LIBGPS_INCLUDE_DIR)

if(LIBGPS_FOUND)
  set(LIBGPS_LIBRARIES ${LIBGPS_LIBRARY})
endif(LIBGPS_FOUND)

mark_as_advanced(LIBGPS_LIBRARY LIBGPS_INCLUDE_DIR)
