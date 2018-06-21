#
# Copyright 2010-2013,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# List of manual sources
########################################################################

########################################################################
# Setup general Doxygen variables
########################################################################
FIND_PACKAGE(Doxygen)
SET(ENABLE_MANUAL_OR_DOXYGEN false)

########################################################################
# Setup Manual (using Doxygen)
########################################################################
LIBUHD_REGISTER_COMPONENT("Manual" ENABLE_MANUAL ON "DOXYGEN_FOUND" OFF OFF)

# Check if fpga submodule is there. If yes:
# - Add fpga-src/docs and fpga-src/README.md to inputs
# - Add fpga-src docs dirs to strip?
# If no:
# - Make FPGA Manual reference link to our website

IF(ENABLE_MANUAL)
    SET(ENABLE_MANUAL_OR_DOXYGEN true)
    # First: Set up UHD manual only
    FILE(GLOB manual_sources "*.dox")
    SET(DOXYGEN_DEPENDENCIES ${manual_sources})
    SET(DOXYGEN_INPUT_DIRS "${CMAKE_SOURCE_DIR}/docs ${CMAKE_CURRENT_BINARY_DIR}")
    SET(DOXYGEN_DEP_COMPONENT "manual")
    SET(DOXYGEN_FPGA_MANUAL_REFERENCE "<a href=\"http://files.ettus.com/manual/md_fpga.html\">Part III: FPGA Manual</a>")
    SET(DOXYGEN_STRIP_EXTRA "")
    SET(DOXYGEN_EXCLUDE_DIRS "")
    IF(NOT ENABLE_RFNOC)
        SET(DOXYGEN_EXCLUDE_DIRS "${DOXYGEN_EXCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include/uhd/rfnoc")
    ENDIF(NOT ENABLE_RFNOC)
    # Now, check if we have the FPGA sources as well.
    # If so, pull them in:
    IF(HAS_FPGA_SUBMODULE)
        FILE(GLOB_RECURSE fpga_manual_sources "${FPGA_SUBMODULE_DIR}/docs/*.md")
        LIST(APPEND manual_sources ${fpga_manual_sources})
        SET(DOXYGEN_INPUT_DIRS "${DOXYGEN_INPUT_DIRS} ${FPGA_SUBMODULE_DIR}/docs")
        SET(DOXYGEN_FPGA_MANUAL_REFERENCE "\\subpage md_fpga \"Part III: FPGA Manual\"")
	SET(DOXYGEN_STRIP_EXTRA "${FPGA_SUBMODULE_DIR}/docs")
    ENDIF(HAS_FPGA_SUBMODULE)
    CONFIGURE_FILE(
        ${CMAKE_CURRENT_SOURCE_DIR}/mainpage.dox.in
        ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox
    @ONLY)
ENDIF(ENABLE_MANUAL)

########################################################################
# Setup API documentation (using Doxygen)
########################################################################
LIBUHD_REGISTER_COMPONENT("API/Doxygen" ENABLE_DOXYGEN ON "DOXYGEN_FOUND" OFF OFF)
OPTION(ENABLE_DOXYGEN_FULL "Use Doxygen to document the entire source tree (not just API)" OFF)
OPTION(ENABLE_DOXYGEN_DOT "Let Doxygen use dot (requires graphviz)" OFF)
OPTION(ENABLE_DOXYGEN_SHORTNAMES "Let Doxygen use shorter filenames (helps on Windows)" OFF)

IF(LIBUHDDEV_PKG)
    SET(PKG_DOC_DIR share/doc/libuhd-dev)
ENDIF(LIBUHDDEV_PKG)

IF(ENABLE_DOXYGEN)
    SET(ENABLE_MANUAL_OR_DOXYGEN true)
    #make doxygen directory depend on the header files
    FILE(GLOB_RECURSE header_files ${CMAKE_SOURCE_DIR}/include/*.hpp)
    FILE(GLOB_RECURSE h_files ${CMAKE_SOURCE_DIR}/include/*.h)
    LIST(APPEND header_files ${h_files})
    SET(DOXYGEN_DEPENDENCIES ${DOXYGEN_DEPENDENCIES} ${header_files} "${CMAKE_SOURCE_DIR}/lib/rfnoc/nocscript/gen_basic_funcs.py")
    IF(ENABLE_DOXYGEN_FULL)
        SET(DOXYGEN_INPUT_DIRS "${DOXYGEN_INPUT_DIRS} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/python")
    ELSE(ENABLE_DOXYGEN_FULL)
        SET(DOXYGEN_INPUT_DIRS "${DOXYGEN_INPUT_DIRS} ${CMAKE_SOURCE_DIR}/include")
    ENDIF(ENABLE_DOXYGEN_FULL)

    SET(DOXYGEN_DEP_COMPONENT "doxygen")
ENDIF(ENABLE_DOXYGEN)

########################################################################
# Run Doxygen (on code and/or manual, depending on CMake flags)
########################################################################
IF(ENABLE_MANUAL_OR_DOXYGEN)
    #generate the doxygen configuration file
    SET(CMAKE_CURRENT_BINARY_DIR_DOXYGEN ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
    IF(ENABLE_DOXYGEN_DOT)
        SET(DOXYGEN_HAVE_DOT "YES")
    ELSE(ENABLE_DOXYGEN_DOT)
        SET(DOXYGEN_HAVE_DOT "NO")
    ENDIF(ENABLE_DOXYGEN_DOT)
    IF(ENABLE_DOXYGEN_SHORTNAMES)
        SET(DOXYGEN_SHORT_NAMES "YES")
    ELSE(ENABLE_DOXYGEN_SHORTNAMES)
        SET(DOXYGEN_SHORT_NAMES "NO")
    ENDIF(ENABLE_DOXYGEN_SHORTNAMES)

    CONFIGURE_FILE(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    @ONLY)

    #make doxygen directory depend on the header files
    FILE(GLOB_RECURSE header_files ${CMAKE_SOURCE_DIR}/include/*.hpp)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN} DEPENDS ${DOXYGEN_DEPENDENCIES}
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMENT "Generating documentation with doxygen"
    )

    #make the doxygen generation a built-time dependency
    ADD_CUSTOM_TARGET(doxygen_docs ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN})
    UHD_INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN} DESTINATION ${PKG_DOC_DIR} COMPONENT ${DOXYGEN_DEP_COMPONENT})
ENDIF(ENABLE_MANUAL_OR_DOXYGEN)

########################################################################
# List of man page sources
########################################################################

SET(man_page_sources
    octoclock_firmware_burner.1
    uhd_cal_rx_iq_balance.1
    uhd_cal_tx_dc_offset.1
    uhd_cal_tx_iq_balance.1
    uhd_config_info.1
    uhd_find_devices.1
    uhd_image_loader.1
    uhd_images_downloader.1
    uhd_usrp_probe.1
    usrp_n2xx_simple_net_burner.1
    usrp_x3xx_fpga_burner.1
    usrp2_card_burner.1
)

########################################################################
# Setup man pages
########################################################################
FIND_PACKAGE(GZip)

# No elegant way in CMake to reverse a boolean
IF(NOT WIN32)
    SET(NOT_WIN32 TRUE)
ENDIF(NOT WIN32)

LIBUHD_REGISTER_COMPONENT("Man Pages" ENABLE_MAN_PAGES ON "GZIP_FOUND;NOT_WIN32" OFF OFF)

IF(ENABLE_MAN_PAGES)
    #Generate man pages
    FOREACH(manfile ${man_page_sources})
        #make the gzip file depend on the text file
        STRING(REPLACE ".1" "" PROGRAM_NAME "${manfile}")
        SET(gzfile "${CMAKE_CURRENT_BINARY_DIR}/${manfile}.gz")
        SET(manfile "${CMAKE_CURRENT_SOURCE_DIR}/${manfile}")
        ADD_CUSTOM_COMMAND(
            OUTPUT ${gzfile}
            DEPENDS ${manfile}
            COMMAND ${GZIP_EXECUTABLE} -9 -cf ${manfile} > ${gzfile}
            COMMENT "Generating ${PROGRAM_NAME} man page"
        )

        #make the man page target depend on the gz file
        LIST(APPEND man_page_gz_files ${gzfile})
        UHD_INSTALL(FILES ${gzfile} DESTINATION ${PKG_MAN_DIR} COMPONENT manpages)
    ENDFOREACH(manfile ${man_page_sources})

    #make the man pages a build-time dependency
    ADD_CUSTOM_TARGET(man_page_gzips ALL DEPENDS ${man_page_gz_files})
ENDIF(ENABLE_MAN_PAGES)
