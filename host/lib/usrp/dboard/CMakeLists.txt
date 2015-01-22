#
# Copyright 2010-2011 Ettus Research LLC
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

########################################################################
# This file included, use CMake directory variables
########################################################################

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/db_basic_and_lf.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_rfx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_xcvr2450.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_sbx_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_sbx_version3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_sbx_version4.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_cbx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_ubx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_wbx_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_wbx_version2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_wbx_version3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_wbx_version4.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_wbx_simple.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_dbsrx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_unknown.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_tvrx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_dbsrx2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_tvrx2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db_e3x0.cpp
)

