#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

INSTALL(PROGRAMS
    mpm_shell.py
    mpm_debug.py
    DESTINATION ${RUNTIME_DIR}
)

SET(eeprom_tool_sources)
SET(eeprom_tool_libs)

IF("${MPM_DEVICE}" STREQUAL "n3xx")
    MESSAGE(STATUS "Adding N3XX-specific EEPROM tools...")
    SET(eeprom_tool_libs eeprom.c)
    SET(eeprom_tool_sources
        db-dump.c
        db-id.c
        db-init.c
        eeprom-blank.c
        eeprom-dump.c
        eeprom-id.c
        eeprom-init.c
        eeprom-set-flags.c
        fan-limits.c
    )
ENDIF("${MPM_DEVICE}" STREQUAL "n3xx")

#for each source: build an executable and install
FOREACH(eeprom_tool_source ${eeprom_tool_sources})
    GET_FILENAME_COMPONENT(eeprom_tool ${eeprom_tool_source} NAME_WE)
    ADD_EXECUTABLE(${eeprom_tool} ${eeprom_tool_source} ${eeprom_tool_libs})
    INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${eeprom_tool} DESTINATION ${RUNTIME_DIR})
ENDFOREACH(eeprom_tool_source ${eeprom_tool_sources})
