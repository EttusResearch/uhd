# Copyright 2011,2012,2016 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

list(APPEND usrptest_sources
    measurement_sink_f_impl.cc
)

set(usrptest_sources "${usrptest_sources}" PARENT_SCOPE)
if(NOT usrptest_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT usrptest_sources)

add_library(gnuradio-usrptest SHARED ${usrptest_sources})
target_link_libraries(gnuradio-usrptest ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES})
set_target_properties(gnuradio-usrptest PROPERTIES DEFINE_SYMBOL "gnuradio_usrptest_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-usrptest PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
include(GrMiscUtils)
GR_LIBRARY_FOO(gnuradio-usrptest RUNTIME_COMPONENT "usrptest_runtime" DEVEL_COMPONENT "usrptest_devel")

########################################################################
# Print summary
########################################################################
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building for version: ${VERSION} / ${LIBVER}")

