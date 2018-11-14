#
# Copyright 2017 Ettus Research
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# - Find liberio
# Find the liberio includes and client library
# This module defines
#  LIBERIO_INCLUDE_DIR, where to find liberio/dma.h
#  LIBERIO_LIBRARIES, the libraries needed by a liberio client.
#  LIBERIO_FOUND, If false, do not try to use liberio.
# also defined, but not for general use are
#  LIBERIO_LIBRARY, where to find the liberio library.

find_package(PkgConfig)
PKG_CHECK_MODULES(PC_LIBERIO QUIET liberio >= 0.3)

find_path(LIBERIO_INCLUDE_DIR liberio/liberio.h
	HINTS $ENV{LIBERIO_DIR}/include ${PC_LIBERIO_INCLUDE_DIR}
	PATH_SUFFIXES liberio
)

find_library(LIBERIO_LIBRARY
	NAMES erio liberio
	HINTS $ENV{LIBERIO_DIR}/lib ${PC_LIBERIO_LIBDIR} ${PC_LIBERIO_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBERIO DEFAULT_MSG LIBERIO_LIBRARY LIBERIO_INCLUDE_DIR)
mark_as_advanced(LIBERIO_INCLUDE_DIR LIBERIO_LIBRARY)

set(LIBERIO_LIBRARIES ${LIBERIO_LIBRARY})
set(LIBERIO_INCLUDE_DIRS ${LIBERIO_INCLUDE_DIR})

