#
# Copyright 2014-2015 Ettus Research LLC
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

UHD_INSTALL(FILES
    octoclock_eeprom.hpp
    multi_usrp_clock.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/usrp_clock
    COMPONENT headers
)

IF(ENABLE_C_API)
    UHD_INSTALL(FILES
    usrp_clock.h
    DESTINATION ${INCLUDE_DIR}/uhd/usrp_clock
    COMPONENT headers
    )
ENDIF(ENABLE_C_API)
