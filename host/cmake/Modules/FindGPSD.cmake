#
# Copyright 2015 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# - Find libgps
# Find the Gpsd includes and client library
# This module defines
#  LIBGPS_INCLUDE_DIR, where to find gps.h
#  LIBGPS_LIBRARIES, the libraries needed by a GPSD client.
#  LIBGPS_FOUND, If false, do not try to use GPSD.
# also defined, but not for general use are
#  LIBGPS_LIBRARY, where to find the GPSD library.

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_GPSD "libgps")
PKG_CHECK_MODULES(PC_GPSD_V3_11 "libgps >= 3.11")

IF(PC_GPSD_FOUND AND NOT PC_GPSD_V3_11_FOUND)
    MESSAGE(WARNING "GPSD version found is too old")
ENDIF(PC_GPSD_FOUND AND NOT PC_GPSD_V3_11_FOUND)

IF(PC_GPSD_V3_11_FOUND)
  FIND_PATH(
      LIBGPS_INCLUDE_DIR
      NAMES gps.h
      HINTS ${PC_GPSD_INCLUDE_DIR}
  )

  SET(
      LIBGPS_NAMES
      ${LIBGPS_NAMES} gps
  )

  FIND_LIBRARY(
      LIBGPS_LIBRARY
      NAMES ${LIBGPS_NAMES}
      HINTS ${PC_GPSD_LIBDIR}
  )
ENDIF(PC_GPSD_V3_11_FOUND)

# handle the QUIETLY and REQUIRED arguments and set LIBGPS_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBGPS DEFAULT_MSG LIBGPS_LIBRARY LIBGPS_INCLUDE_DIR)

IF(LIBGPS_FOUND)
  SET(LIBGPS_LIBRARIES ${LIBGPS_LIBRARY})
ENDIF(LIBGPS_FOUND)

MARK_AS_ADVANCED(LIBGPS_LIBRARY LIBGPS_INCLUDE_DIR)
