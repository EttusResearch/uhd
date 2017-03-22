#
# Copyright 2017 Ettus Research (National Instruments)
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

SET(USRP_MPM_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/types.py
    ${CMAKE_CURRENT_SOURCE_DIR}/discovery.py
    ${CMAKE_CURRENT_SOURCE_DIR}/rpc_process.py
    ${CMAKE_CURRENT_SOURCE_DIR}/rpc_server.py
    ${CMAKE_CURRENT_SOURCE_DIR}/periphs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/periph_manager
    ${CMAKE_CURRENT_SOURCE_DIR}/dboard_manager
    PARENT_SCOPE
    )
