# Copyright 2010-2013 Ettus Research LLC
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

# Set component parameters
set(ETTUS_DISSECTOR_INCLUDE_DIRS ${CMAKE_SOURCE_DIR} CACHE INTERNAL "" FORCE)
set(ETTUS_DISSECTOR_NAME "chdr" CACHE STRING "Select a dissector to build")

function(generate_ettus_dissector DISSECTOR_NAME)

    set(ETTUS_PLUGIN_SRC ${CMAKE_SOURCE_DIR}/packet-${DISSECTOR_NAME}.c)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/moduleinfo.h.in
        ${CMAKE_BINARY_DIR}/moduleinfo.h
    )

    set(PLUGIN_C_GENERATOR ${CMAKE_SOURCE_DIR}/make-dissector-reg-ws1.py)
    set(PLUGIN_C plugin-chdr.c)

    add_custom_command(
            OUTPUT ${PLUGIN_C}
            DEPENDS ${ETTUS_PLUGIN_SRC}
            COMMAND ${PLUGIN_C_GENERATOR} ${CMAKE_SOURCE_DIR} plugin ${DISSECTOR_NAME} ${ETTUS_PLUGIN_SRC}
            COMMENT "Generating ${PLUGIN_C}"
    )

    set(ETTUS_TARGET_NAME "${DISSECTOR_NAME}")
    add_library(${ETTUS_TARGET_NAME} MODULE
        ${PLUGIN_C}
        moduleinfo.h
        ${ETTUS_PLUGIN_SRC}
    )
    set_target_properties(${ETTUS_TARGET_NAME} PROPERTIES PREFIX "")
    set_target_properties(${ETTUS_TARGET_NAME} PROPERTIES LINK_FLAGS "${WS_LINK_FLAGS}")
    target_link_libraries(${ETTUS_TARGET_NAME} ${WIRESHARK_LIBRARIES})

    install(TARGETS ${ETTUS_TARGET_NAME} 
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/plugins NAMELINK_SKIP
    ) 

endfunction(generate_ettus_dissector)

generate_ettus_dissector("${ETTUS_DISSECTOR_NAME}")

