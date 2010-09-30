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
    ${CMAKE_SOURCE_DIR}/lib/usrp/dboard/db_basic_and_lf.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/dboard/db_rfx.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/dboard/db_xcvr2450.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/dboard/db_wbx.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/dboard/db_dbsrx.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/dboard/db_unknown.cpp
    ${CMAKE_SOURCE_DIR}/lib/usrp/dboard/db_tvrx.cpp
)

