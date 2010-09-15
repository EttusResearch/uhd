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

#This file will be included by cmake, use absolute paths!

LIBUHD_APPEND_SOURCES(
    ${CMAKE_SOURCE_DIR}/lib/usrp/dboard_base.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/dboard_eeprom.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/dboard_id.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/dboard_manager.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/dsp_utils.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/mimo_usrp.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/misc_utils.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/simple_usrp.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/subdev_spec.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/tune_helper.cpp
)

INCLUDE(${CMAKE_SOURCE_DIR}/lib/usrp/dboard/CMakeLists.txt)
INCLUDE(${CMAKE_SOURCE_DIR}/lib/usrp/usrp1/CMakeLists.txt)
INCLUDE(${CMAKE_SOURCE_DIR}/lib/usrp/usrp2/CMakeLists.txt)
