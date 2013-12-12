#
# Copyright 2010-2012 Ettus Research LLC
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
SET(util_runtime_sources
    uhd_find_devices.cpp
    uhd_usrp_probe.cpp
    uhd_cal_rx_iq_balance.cpp
    uhd_cal_tx_dc_offset.cpp
    uhd_cal_tx_iq_balance.cpp
    usrp_n2xx_simple_net_burner.cpp
)

#for each source: build an executable and install
FOREACH(util_source ${util_runtime_sources})
    GET_FILENAME_COMPONENT(util_name ${util_source} NAME_WE)
    ADD_EXECUTABLE(${util_name} ${util_source})
    TARGET_LINK_LIBRARIES(${util_name} uhd ${Boost_LIBRARIES})
    UHD_INSTALL(TARGETS ${util_name} RUNTIME DESTINATION ${RUNTIME_DIR} COMPONENT utilities)
ENDFOREACH(util_source)

########################################################################
# Utilities that get installed into the share path
########################################################################
SET(util_share_sources
    query_gpsdo_sensors.cpp
    usrp_burn_db_eeprom.cpp
    usrp_burn_mb_eeprom.cpp
)

IF(ENABLE_USB)
    LIST(APPEND util_share_sources
        fx2_init_eeprom.cpp
        b2xx_fx3_utils.cpp
    )
    INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIRS})
    # Additional include directories for b2xx_fx3_utils
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../lib/usrp/b200 ${CMAKE_CURRENT_SOURCE_DIR}/../lib/usrp/common)
ENDIF(ENABLE_USB)

IF(LINUX AND ENABLE_USB)
    UHD_INSTALL(FILES
        uhd-usrp.rules
        DESTINATION ${PKG_LIB_DIR}/utils
        COMPONENT utilities
    )
ENDIF(LINUX AND ENABLE_USB)

#for each source: build an executable and install
FOREACH(util_source ${util_share_sources})
    GET_FILENAME_COMPONENT(util_name ${util_source} NAME_WE)
    ADD_EXECUTABLE(${util_name} ${util_source})
    TARGET_LINK_LIBRARIES(${util_name} uhd ${Boost_LIBRARIES})
    UHD_INSTALL(TARGETS ${util_name} RUNTIME DESTINATION ${PKG_LIB_DIR}/utils COMPONENT utilities)
ENDFOREACH(util_source)

UHD_INSTALL(TARGETS usrp_n2xx_simple_net_burner RUNTIME DESTINATION ${PKG_LIB_DIR}/utils COMPONENT utilities)

#UHD images downloader configuration
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/uhd_images_downloader.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/uhd_images_downloader.py
@ONLY)
UHD_INSTALL(PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/uhd_images_downloader.py
    DESTINATION ${PKG_LIB_DIR}/utils
    COMPONENT utilities
)
IF(LINUX)
    UHD_INSTALL(PROGRAMS
        ${CMAKE_CURRENT_BINARY_DIR}/uhd_images_downloader.py
        RENAME uhd_images_downloader
        DESTINATION ${RUNTIME_DIR}
        COMPONENT utilities
    )
ENDIF(LINUX)

IF(ENABLE_USRP2)
    SET(burners
        usrp2_card_burner.py
        usrp2_card_burner_gui.py
        usrp_n2xx_net_burner.py
        usrp_n2xx_net_burner_gui.py
    )

    IF(WIN32 AND UHD_RELEASE_MODE) #include dd.exe
        FILE(DOWNLOAD
            "http://files.ettus.com/dd.exe"
            ${CMAKE_CURRENT_BINARY_DIR}/dd.exe
        )
        UHD_INSTALL(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/dd.exe
            DESTINATION ${PKG_LIB_DIR}/utils
            COMPONENT utilities
        )
    ENDIF(WIN32 AND UHD_RELEASE_MODE)
    IF(LINUX)
        UHD_INSTALL(PROGRAMS
            usrp2_recovery.py
            DESTINATION ${PKG_LIB_DIR}/utils
            COMPONENT utilities
        )
        UHD_INSTALL(PROGRAMS
            usrp2_card_burner.py
            RENAME usrp2_card_burner
            DESTINATION ${RUNTIME_DIR}
            COMPONENT utilities
        )
    ENDIF(LINUX)
    FOREACH(burner ${burners})
        UHD_INSTALL(PROGRAMS
            ${burner}
            DESTINATION ${PKG_LIB_DIR}/utils
            COMPONENT utilities
        )
    ENDFOREACH(burner ${burners})

ENDIF(ENABLE_USRP2)
