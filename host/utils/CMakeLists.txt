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

########################################################################
# Utilities that get installed into the runtime path
########################################################################
ADD_EXECUTABLE(uhd_find_devices uhd_find_devices.cpp)
TARGET_LINK_LIBRARIES(uhd_find_devices uhd)

ADD_EXECUTABLE(uhd_usrp_probe uhd_usrp_probe.cpp)
TARGET_LINK_LIBRARIES(uhd_usrp_probe uhd)

ADD_EXECUTABLE(fpga-downloader fpga-downloader.cpp)
TARGET_LINK_LIBRARIES(fpga-downloader)

ADD_EXECUTABLE(clkgen-config clkgen-config.cpp)
TARGET_LINK_LIBRARIES(clkgen-config)

ADD_EXECUTABLE(usrp-e-loopback usrp-e-loopback.c)
TARGET_LINK_LIBRARIES(usrp-e-loopback pthread)

ADD_EXECUTABLE(usrp-e-debug-pins usrp-e-debug-pins.c)
TARGET_LINK_LIBRARIES(usrp-e-debug-pins)

ADD_EXECUTABLE(usrp-e-i2c usrp-e-i2c.c)
TARGET_LINK_LIBRARIES(usrp-e-i2c)

ADD_EXECUTABLE(usrp-e-spi usrp-e-spi.c)
TARGET_LINK_LIBRARIES(usrp-e-spi)

INSTALL(TARGETS
    uhd_find_devices
    uhd_usrp_probe
    fpga-downloader
    usrp-e-loopback
    usrp-e-debug-pins
    usrp-e-spi
    usrp-e-i2c
    clkgen-config
    RUNTIME DESTINATION ${RUNTIME_DIR}
)

########################################################################
# Utilities that get installed into the share path
########################################################################
ADD_EXECUTABLE(usrp2_addr_burner usrp2_addr_burner.cpp)
TARGET_LINK_LIBRARIES(usrp2_addr_burner uhd)

ADD_EXECUTABLE(usrp_burn_db_eeprom usrp_burn_db_eeprom.cpp)
TARGET_LINK_LIBRARIES(usrp_burn_db_eeprom uhd)

INSTALL(TARGETS
    usrp2_addr_burner
    usrp_burn_db_eeprom
    RUNTIME DESTINATION ${PKG_DATA_DIR}/utils
)

INSTALL(PROGRAMS
    usrp2_recovery.py
    usrp2_card_burner.py
    usrp2_card_burner_gui.py
    DESTINATION ${PKG_DATA_DIR}/utils
)
