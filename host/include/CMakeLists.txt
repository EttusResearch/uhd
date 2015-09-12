#
# Copyright 2010,2015 Ettus Research LLC
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

CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

IF(ENABLE_C_API)
    UHD_INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/uhd.h
        DESTINATION ${INCLUDE_DIR}
        COMPONENT headers
    )
ENDIF(ENABLE_C_API)

ADD_SUBDIRECTORY(uhd)
