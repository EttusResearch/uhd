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

SET(USRP_MPM_FILES ${USRP_MPM_FILES})
SET(USRP_MPM_CORE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/tdc_sync.py
    ${CMAKE_CURRENT_SOURCE_DIR}/nijesdcore.py
)
LIST(APPEND USRP_MPM_FILES ${USRP_MPM_CORE_FILES})
SET(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)

