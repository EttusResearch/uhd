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
# List of manual sources
########################################################################
SET(manual_sources
    index.rst
    build.rst
    usrp2.rst
)

########################################################################
# Setup Manual
########################################################################
MESSAGE(STATUS "Checking for rst2html (docutils)")
FIND_PROGRAM(RST2HTML rst2html)
IF(${RST2HTML} STREQUAL "RST2HTML-NOTFOUND")
    MESSAGE(STATUS "Checking for rst2html (docutils) - not found")
ELSE(${RST2HTML} STREQUAL "RST2HTML-NOTFOUND")
    MESSAGE(STATUS "Checking for rst2html (docutils) - found")
    FOREACH(rstfile ${manual_sources})
        SET(rstfile ${CMAKE_CURRENT_SOURCE_DIR}/${rstfile})
        GET_FILENAME_COMPONENT(rstfile_we ${rstfile} NAME_WE)
        SET(htmlfile ${CMAKE_CURRENT_BINARY_DIR}/${rstfile_we}.html)
        ADD_CUSTOM_COMMAND(
            OUTPUT ${htmlfile} DEPENDS ${rstfile} ${CMAKE_CURRENT_SOURCE_DIR}/style.css
            COMMAND ${RST2HTML} ${rstfile} ${htmlfile}
                --stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/style.css --no-toc-backlinks
            COMMENT "Generating ${htmlfile}"
        )
        LIST(APPEND manual_html_files ${htmlfile})
        INSTALL(FILES ${htmlfile} DESTINATION ${PKG_DOC_DIR}/manual/html)
    ENDFOREACH(rstfile ${manual_sources})
    ADD_CUSTOM_TARGET(manual_html ALL DEPENDS ${manual_html_files})
ENDIF(${RST2HTML} STREQUAL "RST2HTML-NOTFOUND")

INSTALL(FILES ${manual_sources} DESTINATION ${PKG_DOC_DIR}/manual/rst)

########################################################################
# Setup Doxygen
########################################################################
INCLUDE(FindDoxygen)

IF(DOXYGEN_FOUND)
    SET(CMAKE_CURRENT_BINARY_DIR_DOXYGEN ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
    CONFIGURE_FILE(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    @ONLY)
    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN}
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMENT "Generating documentation with doxygen"
    )
    ADD_CUSTOM_TARGET(doxygen_html ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN})
    INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN} DESTINATION ${PKG_DOC_DIR})
ENDIF(DOXYGEN_FOUND)
