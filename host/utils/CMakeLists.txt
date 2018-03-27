#
# Copyright 2010-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Utilities that get installed into the runtime path
########################################################################
SET(util_runtime_sources
    uhd_config_info.cpp
    uhd_find_devices.cpp
    uhd_usrp_probe.cpp
    uhd_image_loader.cpp
    uhd_cal_rx_iq_balance.cpp
    uhd_cal_tx_dc_offset.cpp
    uhd_cal_tx_iq_balance.cpp
)

find_package(UDev)
IF(ENABLE_E300 AND NOT E300_FORCE_NETWORK)
    IF(UDEV_FOUND)
        LIST(APPEND util_runtime_sources usrp_e3x0_network_mode.cpp)
    ENDIF(UDEV_FOUND)
ENDIF(ENABLE_E300 AND NOT E300_FORCE_NETWORK)

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
    converter_benchmark.cpp
    query_gpsdo_sensors.cpp
    usrp_burn_db_eeprom.cpp
    usrp_burn_mb_eeprom.cpp
)
SET(util_share_sources_py
    converter_benchmark.py
)
IF(ENABLE_USB)
    LIST(APPEND util_share_sources
        fx2_init_eeprom.cpp
    )
    INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIRS})
    IF(ENABLE_B200)
        LIST(APPEND util_share_sources
            b2xx_fx3_utils.cpp
        )
        # Additional include directories for b2xx_fx3_utils
        INCLUDE_DIRECTORIES(
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/usrp/b200
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/usrp/common
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/usrp/common/ad9361_driver
        )
    ENDIF(ENABLE_B200)
ENDIF(ENABLE_USB)
IF(ENABLE_OCTOCLOCK)
    LIST(APPEND util_share_sources
        octoclock_burn_eeprom.cpp
    )
ENDIF(ENABLE_OCTOCLOCK)

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
FOREACH(util_source ${util_share_sources_py})
    UHD_INSTALL(PROGRAMS
        ${CMAKE_CURRENT_SOURCE_DIR}/${util_source}
        DESTINATION ${PKG_LIB_DIR}/utils
        COMPONENT utilities
    )
ENDFOREACH(util_source)

#UHD images downloader configuration
FILE(READ ${CMAKE_CURRENT_SOURCE_DIR}/../../images/manifest.txt CMAKE_MANIFEST_CONTENTS)
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/uhd_images_downloader.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/uhd_images_downloader.py
@ONLY)
# TODO: FIXME when know how.
# This is a hack to force cmake regenerate uhd_images_downloader.py
# whenever manifest.txt file changed.
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../images/manifest.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/../../images/manifest.txt
COPYONLY)

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
IF(NOT HAVE_PYTHON_MODULE_REQUESTS)
    MESSAGE(WARNING "Python module `requests' not found -- uhd_images_downloader.py will not work without it.")
    MESSAGE(WARNING "You may be able to install this by running 'pip install requests'")
ENDIF(NOT HAVE_PYTHON_MODULE_REQUESTS)

IF(ENABLE_USRP2)
    SET(burners
        usrp2_card_burner.py
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

########################################################################
# Other files that are not utilities or executables
########################################################################
IF(WIN32)
    SET(windows_extra_files
        FastSendDatagramThreshold.reg
    )
    UHD_INSTALL(
        FILES ${windows_extra_files}
        DESTINATION ${PKG_DATA_DIR}
        COMPONENT utilities
    )
ENDIF(WIN32)

ADD_SUBDIRECTORY(latency)
