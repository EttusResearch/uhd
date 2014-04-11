#
# Copyright 2010-2013 Ettus Research LLC
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
MESSAGE(STATUS "")
LIBUHD_REGISTER_COMPONENT("Manual" ENABLE_MANUAL ON "DOXYGEN_FOUND" OFF)

IF(ENABLE_MANUAL)
    SET(ENABLE_MANUAL_OR_DOXYGEN true)
    FILE(GLOB manual_sources "*.dox")
    SET(DOXYGEN_DEPENDENCIES ${manual_sources})
    SET(DOXYGEN_INPUT_DIRS ${CMAKE_SOURCE_DIR}/docs)
    SET(DOXYGEN_DEP_COMPONENT "manual")
ENDIF(ENABLE_MANUAL)

########################################################################
# Setup API documentation (using Doxygen)
########################################################################
MESSAGE(STATUS "")
LIBUHD_REGISTER_COMPONENT("API/Doxygen" ENABLE_DOXYGEN ON "DOXYGEN_FOUND" OFF)
OPTION(ENABLE_DOXYGEN_FULL "Use Doxygen to document the entire source tree (not just API)" OFF)

IF(LIBUHDDEV_PKG)
    SET(PKG_DOC_DIR share/doc/libuhd-dev)
ENDIF(LIBUHDDEV_PKG)

IF(ENABLE_DOXYGEN)
    SET(ENABLE_MANUAL_OR_DOXYGEN true)
    #make doxygen directory depend on the header files
    FILE(GLOB_RECURSE header_files ${CMAKE_SOURCE_DIR}/include/*.hpp)
    SET(DOXYGEN_DEPENDENCIES ${DOXYGEN_DEPENDENCIES} ${header_files})
    IF(ENABLE_DOXYGEN_FULL)
        SET(DOXYGEN_INPUT_DIRS "${DOXYGEN_INPUT_DIRS} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib")
    ELSE(ENABLE_DOXYGEN_FULL)
        SET(DOXYGEN_INPUT_DIRS "${DOXYGEN_INPUT_DIRS} ${CMAKE_SOURCE_DIR}/include")
    ENDIF(ENABLE_DOXYGEN_FULL)

    SET(DOXYGEN_DEP_COMPONENT "doxygen")
ENDIF(ENABLE_DOXYGEN)

########################################################################
# Run Doxygen (on code and/or manual, depending on CMake flags)
########################################################################
MESSAGE(STATUS "")
IF(ENABLE_MANUAL_OR_DOXYGEN)
    #generate the doxygen configuration file
    SET(CMAKE_CURRENT_BINARY_DIR_DOXYGEN ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
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
    uhd_cal_rx_iq_balance.1
    uhd_cal_tx_dc_offset.1
    uhd_cal_tx_iq_balance.1
    uhd_find_devices.1
    uhd_images_downloader.1
    uhd_usrp_probe.1
    usrp_n2xx_simple_net_burner.1
    usrp_x3xx_fpga_burner.1
    usrp2_card_burner.1
)

########################################################################
# Setup man pages
########################################################################
MESSAGE(STATUS "")
FIND_PACKAGE(GZip)

# No elegant way in CMake to reverse a boolean
IF(NOT WIN32)
    SET(NOT_WIN32 TRUE)
ENDIF(NOT WIN32)

LIBUHD_REGISTER_COMPONENT("Man Pages" ENABLE_MAN_PAGES ON "GZIP_FOUND;NOT_WIN32" OFF)

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
