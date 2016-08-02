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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_basic_funcs.py
    ${CMAKE_CURRENT_BINARY_DIR}/basic_functions.hpp
)

IF(ENABLE_MANUAL)
    LIBUHD_PYTHON_GEN_SOURCE(
        ${CMAKE_CURRENT_SOURCE_DIR}/gen_basic_funcs.py
        ${CMAKE_BINARY_DIR}/docs/nocscript_functions.dox
    )
ENDIF(ENABLE_MANUAL)

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/expression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/function_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_iface.cpp
)
