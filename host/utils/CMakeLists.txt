#
# Copyright 2010-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(CONFIG_PATH "${CMAKE_INSTALL_PREFIX}/${PKG_DATA_DIR}")

########################################################################
# First, only utilities which get installed with ENABLE_PYMOD_UTILS.
# These require Mako and ruamel.yaml.
########################################################################
if(ENABLE_PYMOD_UTILS)
    ### RFNoC Image Builder
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_image_builder.py"
        "${CMAKE_CURRENT_BINARY_DIR}/rfnoc_image_builder"
    )
    UHD_INSTALL(PROGRAMS
        ${CMAKE_CURRENT_BINARY_DIR}/rfnoc_image_builder
        RENAME rfnoc_image_builder
        DESTINATION ${RUNTIME_DIR}
        COMPONENT utilities
    )

    ### RFNoC modtool. Note this also requires the RFNoC gain OOT.
    UHD_INSTALL(PROGRAMS
        ${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_modtool.py
        RENAME rfnoc_modtool
        DESTINATION ${RUNTIME_DIR}
        COMPONENT utilities
    )

    file(GLOB_RECURSE RFNOC_GAIN_OOT_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain/apps/CMakeLists.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain/cmake/*
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain/CMakeLists.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain/examples/CMakeLists.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain/fpga/gain/CMakeLists.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain/include/rfnoc/gain/CMakeLists.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain/lib/CMakeLists.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain/python/*
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain/README.md
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain/rfnoc/CMakeLists.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain/rfnoc/blocks/CMakeLists.txt
        )

    add_custom_target(
        rfnoc_newmod ALL
        DEPENDS ${RFNOC_GAIN_OOT_FILES}
        COMMAND
        "${PYTHON_EXECUTABLE}" "${UHD_SOURCE_DIR}/cmake/Modules/copy_rfnoc_newmod.py"
        "--source" "${CMAKE_CURRENT_SOURCE_DIR}/../examples/rfnoc-gain"
        "--dest" "${CMAKE_CURRENT_BINARY_DIR}/rfnoc-newmod"
        "--module-dir" "${CMAKE_CURRENT_SOURCE_DIR}/../python/uhd"
        COMMENT "Generating rfnoc-newmod"
    )
    install(DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/rfnoc-newmod
        DESTINATION ${PKG_DATA_DIR}
        COMPONENT utilities)
endif(ENABLE_PYMOD_UTILS)

########################################################################
# Now, utilities which do not depend on Mako or ruamel.yaml
########################################################################
# Decide if we install all utilities into the runtime path
if(LINUX OR BSD OR CMAKE_SYSTEM_NAME STREQUAL "GNU")
    set(UHD_INSTALL_ALL_UTILS TRUE)
else()
    set(UHD_INSTALL_ALL_UTILS FALSE)
endif()


# This section must only include Python utilities that do not depend on uhd
if(ENABLE_PYMOD_UTILS OR ENABLE_UTILS)
    ### UHD Images downloader
    # Configure the scripts
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../../images/manifest.txt CMAKE_MANIFEST_CONTENTS)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/uhd_images_downloader.py.in
        ${CMAKE_CURRENT_BINARY_DIR}/uhd_images_downloader.py
    @ONLY)
    # TODO: FIXME when know how.
    # This is a hack to force cmake regenerate uhd_images_downloader.py
    # whenever manifest.txt file changed.
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../images/manifest.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/../../images/manifest.txt
    COPYONLY)

    UHD_INSTALL(PROGRAMS
        ${CMAKE_CURRENT_BINARY_DIR}/uhd_images_downloader.py
        DESTINATION ${PKG_LIB_DIR}/utils
        COMPONENT utilities
    )
    # On Linux/Unix systems, also install into $PATH
    if(UHD_INSTALL_ALL_UTILS)
        UHD_INSTALL(PROGRAMS
            ${CMAKE_CURRENT_BINARY_DIR}/uhd_images_downloader.py
            RENAME uhd_images_downloader
            DESTINATION ${RUNTIME_DIR}
            COMPONENT utilities
        )
    endif()
    if(NOT HAVE_PYTHON_MODULE_REQUESTS)
        message(WARNING
            "Python module `requests' not found -- uhd_images_downloader.py "
            "will not work without it.")
        message(WARNING
            "You may be able to install this by running 'pip install requests'")
    endif(NOT HAVE_PYTHON_MODULE_REQUESTS)

    ### USRP2 Card Burner
    if(ENABLE_USRP2)
        if(WIN32 AND UHD_RELEASE_MODE) #include dd.exe
            file(DOWNLOAD
                "http://files.ettus.com/dd.exe"
                ${CMAKE_CURRENT_BINARY_DIR}/dd.exe
            )
            UHD_INSTALL(FILES
                ${CMAKE_CURRENT_BINARY_DIR}/dd.exe
                DESTINATION ${PKG_LIB_DIR}/utils
                COMPONENT utilities
            )
        endif(WIN32 AND UHD_RELEASE_MODE)
        if(UHD_INSTALL_ALL_UTILS)
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
        endif()
        UHD_INSTALL(PROGRAMS
            usrp2_card_burner.py
            DESTINATION ${PKG_LIB_DIR}/utils
            COMPONENT utilities
        )
    endif(ENABLE_USRP2)

endif(ENABLE_PYMOD_UTILS OR ENABLE_UTILS)

if(NOT ENABLE_UTILS)
    return()
endif()

########################################################################
# Utilities that get installed into the runtime path
########################################################################
set(util_runtime_sources
    uhd_config_info.cpp
    uhd_find_devices.cpp
    uhd_usrp_probe.cpp
    uhd_image_loader.cpp
    uhd_cal_rx_iq_balance.cpp
    uhd_cal_tx_dc_offset.cpp
    uhd_cal_tx_iq_balance.cpp
    uhd_adc_self_cal.cpp
)

#for each source: build an executable and install
foreach(util_source ${util_runtime_sources})
    get_filename_component(util_name ${util_source} NAME_WE)
    add_executable(${util_name} ${util_source})
    target_link_libraries(${util_name} uhd ${Boost_LIBRARIES})
    UHD_INSTALL(TARGETS ${util_name} RUNTIME DESTINATION ${RUNTIME_DIR} COMPONENT utilities)
endforeach(util_source)

if(ENABLE_PYTHON_API)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/usrpctl.py"
        "${CMAKE_CURRENT_BINARY_DIR}/usrpctl"
    )
    UHD_INSTALL(PROGRAMS
        ${CMAKE_CURRENT_BINARY_DIR}/usrpctl
        RENAME usrpctl
        DESTINATION ${RUNTIME_DIR}
        COMPONENT utilities
    )
endif()

########################################################################
# Utilities that get installed into the share path
########################################################################
set(util_share_sources
    converter_benchmark.cpp
    query_gpsdo_sensors.cpp
    usrp_burn_db_eeprom.cpp
    usrp_burn_mb_eeprom.cpp
)
set(util_share_sources_py
    converter_benchmark.py
)
if(ENABLE_USB)
    find_package(LIBUSB)
    list(APPEND util_share_sources
        fx2_init_eeprom.cpp
    )
    include_directories(${LIBUSB_INCLUDE_DIRS})
    if(ENABLE_B200)
        list(APPEND util_share_sources
            b2xx_fx3_utils.cpp
        )
        # Additional include directories for b2xx_fx3_utils
        include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/usrp/b200
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/usrp/common
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/usrp/common/ad9361_driver
        )
    endif(ENABLE_B200)
endif(ENABLE_USB)
if(ENABLE_OCTOCLOCK)
    list(APPEND util_share_sources
        octoclock_burn_eeprom.cpp
    )
endif(ENABLE_OCTOCLOCK)

if(LINUX AND ENABLE_USB)
    UHD_INSTALL(FILES
        uhd-usrp.rules
        DESTINATION ${PKG_LIB_DIR}/utils
        COMPONENT utilities
    )
endif(LINUX AND ENABLE_USB)

#for each source: build an executable and install
foreach(util_source ${util_share_sources})
    get_filename_component(util_name ${util_source} NAME_WE)
    add_executable(${util_name} ${util_source})
    target_link_libraries(${util_name} uhd ${Boost_LIBRARIES})
    UHD_INSTALL(TARGETS ${util_name} RUNTIME DESTINATION ${PKG_LIB_DIR}/utils COMPONENT utilities)
endforeach(util_source)
foreach(util_source ${util_share_sources_py})
    UHD_INSTALL(PROGRAMS
        ${CMAKE_CURRENT_SOURCE_DIR}/${util_source}
        DESTINATION ${PKG_LIB_DIR}/utils
        COMPONENT utilities
    )
endforeach(util_source)

########################################################################
# Other files that are not utilities or executables
########################################################################
if(WIN32)
    set(windows_extra_files
        FastSendDatagramThreshold.reg
    )
    UHD_INSTALL(
        FILES ${windows_extra_files}
        DESTINATION ${PKG_DATA_DIR}
        COMPONENT utilities
    )
endif(WIN32)

add_subdirectory(latency)
