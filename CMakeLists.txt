#
# Copyright 2010 Ettus Research LLC
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

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(UHD)
ENABLE_TESTING()
INCLUDE(CPack)

########################################################################
# Local Include Dir
########################################################################
INCLUDE_DIRECTORIES(${UHD_SOURCE_DIR}/include)

########################################################################
# Setup Boost
########################################################################
FIND_PACKAGE(Boost 1.36 REQUIRED
    date_time
    program_options
    system
    thread
    unit_test_framework
)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

########################################################################
# Add the subdirectories
########################################################################
ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(apps)
