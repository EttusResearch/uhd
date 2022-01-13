#
# Copyright 2010-2013,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# List of manual sources
########################################################################

########################################################################
# Setup general Doxygen variables
########################################################################
find_package(Doxygen)
set(ENABLE_MANUAL_OR_DOXYGEN false)

########################################################################
# Setup Manual (using Doxygen)
########################################################################
LIBUHD_REGISTER_COMPONENT("Manual" ENABLE_MANUAL ON "DOXYGEN_FOUND" OFF OFF)

if(ENABLE_MANUAL)
    set(FPGA_SOURCE_DIR "${UHD_SOURCE_DIR}/../fpga")
    set(ENABLE_MANUAL_OR_DOXYGEN true)
    file(GLOB manual_sources "*.dox")
    file(GLOB_RECURSE fpga_manual_sources "${FPGA_SOURCE_DIR}/docs/*.md")
    list(APPEND manual_sources ${fpga_manual_sources})
    set(DOXYGEN_DEPENDENCIES ${manual_sources})
    set(DOXYGEN_INPUT_DIRS "${UHD_SOURCE_DIR}/docs ${CMAKE_CURRENT_BINARY_DIR} ${FPGA_SOURCE_DIR}/docs")
    set(DOXYGEN_DEP_COMPONENT "manual")
    set(DOXYGEN_FPGA_MANUAL_REFERENCE "\\subpage md_fpga \"Part III: FPGA Manual\"")
    set(DOXYGEN_STRIP_EXTRA "${UHD_SOURCE_DIR}/../fpga/docs")
    set(DOXYGEN_EXCLUDE_DIRS "")
endif(ENABLE_MANUAL)

########################################################################
# Setup API documentation (using Doxygen)
########################################################################
LIBUHD_REGISTER_COMPONENT("API/Doxygen" ENABLE_DOXYGEN ON "DOXYGEN_FOUND" OFF OFF)
option(ENABLE_DOXYGEN_FULL "Use Doxygen to document the entire source tree (not just API)" OFF)
option(ENABLE_DOXYGEN_DOT "Let Doxygen use dot (requires graphviz)" OFF)
option(ENABLE_DOXYGEN_SHORTNAMES "Let Doxygen use shorter filenames (helps on Windows)" OFF)
set(
    MATHJAX_RELPATH https://cdn.jsdelivr.net/npm/mathjax@2/
    CACHE STRING "MATHJAX_RELPATH option for Doxygen")
mark_as_advanced(MATHJAX_RELPATH)

if(LIBUHDDEV_PKG)
    set(PKG_DOC_DIR share/doc/libuhd-dev)
endif(LIBUHDDEV_PKG)

if(ENABLE_DOXYGEN)
    set(ENABLE_MANUAL_OR_DOXYGEN true)
    #make doxygen directory depend on the header files
    file(GLOB_RECURSE header_files ${UHD_SOURCE_DIR}/include/*.hpp)
    file(GLOB_RECURSE h_files ${UHD_SOURCE_DIR}/include/*.h)
    list(APPEND header_files ${h_files})
    set(DOXYGEN_DEPENDENCIES ${DOXYGEN_DEPENDENCIES} ${header_files})
    if(ENABLE_DOXYGEN_FULL)
        set(DOXYGEN_INPUT_DIRS "${DOXYGEN_INPUT_DIRS} ${UHD_SOURCE_DIR}/include ${UHD_SOURCE_DIR}/lib ${UHD_SOURCE_DIR}/python")
    else(ENABLE_DOXYGEN_FULL)
        set(DOXYGEN_INPUT_DIRS "${DOXYGEN_INPUT_DIRS} ${UHD_SOURCE_DIR}/include")
    endif(ENABLE_DOXYGEN_FULL)

    set(DOXYGEN_DEP_COMPONENT "doxygen")
endif(ENABLE_DOXYGEN)

########################################################################
# Run Doxygen (on code and/or manual, depending on CMake flags)
########################################################################
if(ENABLE_MANUAL_OR_DOXYGEN)
    #generate the doxygen configuration file
    set(CMAKE_CURRENT_BINARY_DIR_DOXYGEN ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
    if(ENABLE_DOXYGEN_DOT)
        set(DOXYGEN_HAVE_DOT "YES")
    else(ENABLE_DOXYGEN_DOT)
        set(DOXYGEN_HAVE_DOT "NO")
    endif(ENABLE_DOXYGEN_DOT)
    if(ENABLE_DOXYGEN_SHORTNAMES)
        set(DOXYGEN_SHORT_NAMES "YES")
    else(ENABLE_DOXYGEN_SHORTNAMES)
        set(DOXYGEN_SHORT_NAMES "NO")
    endif(ENABLE_DOXYGEN_SHORTNAMES)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    @ONLY)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/build.dox.in
        ${CMAKE_CURRENT_BINARY_DIR}/build.dox
    @ONLY)

    #make doxygen directory depend on the header files
    file(GLOB_RECURSE header_files ${UHD_SOURCE_DIR}/include/*.hpp)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN} DEPENDS ${DOXYGEN_DEPENDENCIES}
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMENT "Generating documentation with doxygen"
    )

    #make the doxygen generation a built-time dependency
    add_custom_target(doxygen_docs ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN})
    UHD_INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN} DESTINATION ${PKG_DOC_DIR} COMPONENT ${DOXYGEN_DEP_COMPONENT})
endif(ENABLE_MANUAL_OR_DOXYGEN)

########################################################################
# List of man page sources
########################################################################

set(man_page_sources
    uhd_cal_rx_iq_balance.1
    uhd_cal_tx_dc_offset.1
    uhd_cal_tx_iq_balance.1
    uhd_config_info.1
    uhd_find_devices.1
    uhd_image_loader.1
    uhd_images_downloader.1
    uhd_usrp_probe.1
    usrp_n2xx_simple_net_burner.1
    usrp2_card_burner.1
)

if (ENABLE_PYTHON_API)
    set(man_page_sources
        usrpctl.1
    )
endif(ENABLE_PYTHON_API)

########################################################################
# Setup man pages
########################################################################
# No elegant way in CMake to reverse a boolean
if(NOT WIN32)
    set(NOT_WIN32 TRUE)
endif(NOT WIN32)

set(MAN_PAGES_DEPS "NOT_WIN32")

# Quietly see if we have GZip, if not, we can skip trying to compress
# the man pages
find_package(GZip QUIET)
set(ENABLE_MAN_PAGE_COMPRESSION ${GZIP_FOUND} CACHE BOOL "Compress Man Pages")

message(STATUS "")
if(ENABLE_MAN_PAGE_COMPRESSION)
    # If gzip is not found, this will force failure in
    # LIBUHD_REGISTER_COMPONENT for ENABLE_MAN_PAGES
    list(APPEND MAN_PAGES_DEPS "GZIP_FOUND")
    if(GZIP_FOUND)
        message(STATUS "Found GZip: ${GZIP_EXECUTABLE}")
        message(STATUS "")
        message(STATUS "Compressed Man Pages enabled")
    else()
        message(FATAL_ERROR
                "ENABLE_MAN_PAGE_COMPRESSION is set, "
                "but GZip compression program not found!")
        message(STATUS "")
    endif()
else()
    message(STATUS "Compressed Man Pages disabled")
endif(ENABLE_MAN_PAGE_COMPRESSION)
message(STATUS "  Override with -DENABLE_MAN_PAGE_COMPRESSION=ON/OFF")

LIBUHD_REGISTER_COMPONENT("Man Pages" ENABLE_MAN_PAGES ON "${MAN_PAGES_DEPS}" OFF OFF)
if(ENABLE_MAN_PAGES)
    #Generate man pages; either compressed or not
    if(ENABLE_MAN_PAGE_COMPRESSION)
        # compress man pages
        foreach(manfile ${man_page_sources})
            #make the gzip file depend on the text file
            string(REPLACE ".1" "" PROGRAM_NAME "${manfile}")
            set(gzfile "${CMAKE_CURRENT_BINARY_DIR}/${manfile}.gz")
            set(manfile "${CMAKE_CURRENT_SOURCE_DIR}/${manfile}")
            add_custom_command(
                OUTPUT ${gzfile}
                DEPENDS ${manfile}
                COMMAND ${GZIP_EXECUTABLE} -9 -cf ${manfile} > ${gzfile}
                COMMENT "Generating ${PROGRAM_NAME} man page"
            )
            #make the man page target depend on the gz file
            list(APPEND man_page_gz_files ${gzfile})
        endforeach(manfile ${man_page_sources})
        #make the man pages a build-time dependency
        UHD_INSTALL(FILES ${man_page_gz_files} DESTINATION ${PKG_MAN_DIR} COMPONENT manpages)
        add_custom_target(man_page_gzips ALL DEPENDS ${man_page_gz_files})
    else(ENABLE_MAN_PAGE_COMPRESSION)
        # uncompressed man pages; just install them
        UHD_INSTALL(FILES ${man_page_sources} DESTINATION ${PKG_MAN_DIR} COMPONENT manpages)
    endif(ENABLE_MAN_PAGE_COMPRESSION)
endif(ENABLE_MAN_PAGES)
