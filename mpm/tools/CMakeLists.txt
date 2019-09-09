#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

install(PROGRAMS
    mpm_shell.py
    mpm_debug.py
    check-filesystem
    DESTINATION ${RUNTIME_DIR}
)

set(eeprom_tool_sources)
set(eeprom_tool_libs)

if(ENABLE_LIBMPM AND NOT ENABLE_E300 AND NOT ENABLE_X400)
    message(STATUS "Adding MPM EEPROM tools...")
    set(eeprom_tool_libs eeprom.c)
    list(APPEND eeprom_tool_sources
        eeprom-blank.c
        eeprom-dump.c
        eeprom-id.c
        eeprom-init.c
        eeprom-set-flags.c
    )
endif(ENABLE_LIBMPM AND NOT ENABLE_E300 AND NOT ENABLE_X400)
if(ENABLE_MYKONOS)
    message(STATUS "Adding N3XX-specific EEPROM tools...")
    set(eeprom_tool_libs eeprom.c)
    list(APPEND eeprom_tool_sources
        db-dump.c
        db-id.c
        db-init.c
        fan-limits.c
    )
endif(ENABLE_MYKONOS)


#for each source: build an executable and install
foreach(eeprom_tool_source ${eeprom_tool_sources})
    get_filename_component(eeprom_tool ${eeprom_tool_source} NAME_WE)
    add_executable(${eeprom_tool} ${eeprom_tool_source} ${eeprom_tool_libs})
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${eeprom_tool} DESTINATION ${RUNTIME_DIR})
endforeach(eeprom_tool_source ${eeprom_tool_sources})

add_subdirectory(tlv_eeprom)
