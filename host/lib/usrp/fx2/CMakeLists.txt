#
# Copyright 2011 Ettus Research LLC
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
IF(ENABLE_USRP1)
    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/../firmware/fx2/common)
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/fx2_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fx2_ctrl.hpp
    )
ENDIF(ENABLE_USRP1)
