#
# Copyright 2010-2012 Ettus Research LLC
# Copyright 2019 Ettus Research, a National Instruments Brand
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

cmake_minimum_required(VERSION 3.5.1)
project(kitchen_sink CXX)

###############################################################################

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

### Set up build environment ##################################################
#### Set up UHD:
find_package(UHD 3.15.0.0 REQUIRED)

#### Set up Boost:
set(UHD_BOOST_REQUIRED_COMPONENTS
    date_time
    filesystem
    program_options
    regex
    system
    thread
    unit_test_framework
)
set(BOOST_MIN_VERSION "1.58")
include(UHDBoost)

### Configure Compiler ########################################################
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${UHD_INCLUDE_DIRS}
)

### Make the executable #######################################################
add_executable(kitchen_sink kitchen_sink.cpp)
target_link_libraries(kitchen_sink ${UHD_LIBRARIES} ${Boost_LIBRARIES})

# Skip installing.
