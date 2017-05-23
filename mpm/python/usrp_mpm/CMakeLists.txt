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
SET(USRP_MPM_FILES ${USRP_MPM_FILES})
SET(USRP_MPM_TOP_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/discovery.py
    ${CMAKE_CURRENT_SOURCE_DIR}/dtoverlay.py
    ${CMAKE_CURRENT_SOURCE_DIR}/mpmlog.py
    ${CMAKE_CURRENT_SOURCE_DIR}/nijesdcore.py
    ${CMAKE_CURRENT_SOURCE_DIR}/rpc_server.py
    ${CMAKE_CURRENT_SOURCE_DIR}/sysfs_gpio.py
    ${CMAKE_CURRENT_SOURCE_DIR}/mpmtypes.py
    ${CMAKE_CURRENT_SOURCE_DIR}/net.py
    ${CMAKE_CURRENT_SOURCE_DIR}/uio.py
)
LIST(APPEND USRP_MPM_FILES ${USRP_MPM_TOP_FILES})
ADD_SUBDIRECTORY(periph_manager)
ADD_SUBDIRECTORY(dboard_manager)
ADD_SUBDIRECTORY(chips)
SET(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)
