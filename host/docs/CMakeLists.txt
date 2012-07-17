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
# List of manual sources
########################################################################
SET(manual_sources
    index.rst
    identification.rst
    build.rst
    calibration.rst
    coding.rst
    dboards.rst
    gpsdo.rst
    general.rst
    images.rst
    stream.rst
    sync.rst
    transport.rst
    usrp1.rst
    usrp2.rst
    usrp_b100.rst
    usrp_e1x0.rst
)

########################################################################
# Setup Manual
########################################################################
MESSAGE(STATUS "")
FIND_PACKAGE(Docutils)

LIBUHD_REGISTER_COMPONENT("Manual" ENABLE_MANUAL ON "DOCUTILS_FOUND" OFF)

IF(ENABLE_MANUAL)
    #setup rst2html options
    SET(stylesheet ${CMAKE_CURRENT_SOURCE_DIR}/style.css)
    SET(rst2html_options
        --stylesheet=${stylesheet}
        --no-toc-backlinks --date --time
    )

    #create generation rule for each source
    FOREACH(rstfile ${manual_sources})
        #set input and output file names
        SET(rstfile ${CMAKE_CURRENT_SOURCE_DIR}/${rstfile})
        GET_FILENAME_COMPONENT(rstfile_we ${rstfile} NAME_WE)
        SET(htmlfile ${CMAKE_CURRENT_BINARY_DIR}/${rstfile_we}.html)

        #make the html file depend on the rst file
        ADD_CUSTOM_COMMAND(
            OUTPUT ${htmlfile} DEPENDS ${rstfile} ${stylesheet}
            COMMAND ${RST2HTML_EXECUTABLE} ${rstfile} ${htmlfile} ${rst2html_options}
            COMMENT "Generating ${htmlfile}"
        )

        #make the manual target depend on the html file
        LIST(APPEND manual_html_files ${htmlfile})
        INSTALL(FILES ${htmlfile} DESTINATION ${PKG_DOC_DIR}/manual/html COMPONENT manual)
    ENDFOREACH(rstfile ${manual_sources})

    #make the html manual a build-time dependency
    ADD_CUSTOM_TARGET(manual_html ALL DEPENDS ${manual_html_files})
ENDIF(ENABLE_MANUAL)

INSTALL(FILES ${manual_sources} DESTINATION ${PKG_DOC_DIR}/manual/rst COMPONENT manual)

########################################################################
# Setup Doxygen
########################################################################
MESSAGE(STATUS "")
FIND_PACKAGE(Doxygen)

LIBUHD_REGISTER_COMPONENT("Doxygen" ENABLE_DOXYGEN ON "DOXYGEN_FOUND" OFF)

IF(ENABLE_DOXYGEN)
    #generate the doxygen configuration file
    SET(CMAKE_CURRENT_BINARY_DIR_DOXYGEN ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
    CONFIGURE_FILE(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    @ONLY)

    #make doxygen directory depend on the header files
    FILE(GLOB_RECURSE header_files ${CMAKE_SOURCE_DIR}/include/*.hpp)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN} DEPENDS ${header_files}
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMENT "Generating documentation with doxygen"
    )

    #make the doxygen generation a built-time dependency
    ADD_CUSTOM_TARGET(doxygen_docs ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN})
    INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN} DESTINATION ${PKG_DOC_DIR} COMPONENT doxygen)
ENDIF(ENABLE_DOXYGEN)
