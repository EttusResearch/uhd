#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set(eeprom_tool_sources)
set(eeprom_tool_libs)

if(ENABLE_LIBMPM AND ENABLE_X400)
    message(STATUS "Adding MPM EEPROM tools...")
    set(eeprom_tool_libs tlv_eeprom.c tlv_eeprom_io.c crc.c eeprom-pids.c)
    list(APPEND eeprom_tool_sources
        eeprom-dump.c
        eeprom-id.c
        eeprom-init.c
    )

    add_executable(eeprom-update-core eeprom-init.c ${eeprom_tool_libs})
    target_compile_definitions(eeprom-update-core PRIVATE -DTLV_EEPROM_UPDATE)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/eeprom-update-core DESTINATION ${RUNTIME_DIR})
    add_custom_target(eeprom-update ALL COMMAND ${CMAKE_COMMAND} -E create_symlink eeprom-wrapper eeprom-update)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/eeprom-update DESTINATION ${RUNTIME_DIR})
endif(ENABLE_LIBMPM AND ENABLE_X400)

foreach(eeprom_tool_source ${eeprom_tool_sources})
    get_filename_component(eeprom_tool ${eeprom_tool_source} NAME_WE)
    # install eeprom-id, eeprom-dump, etc. with "-core" appendix
    add_executable(${eeprom_tool}-core ${eeprom_tool_source} ${eeprom_tool_libs})
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${eeprom_tool}-core DESTINATION ${RUNTIME_DIR})
    # install eeprom-id, eeprom-dump, etc. wrappers
    add_custom_target(${eeprom_tool} ALL COMMAND ${CMAKE_COMMAND} -E create_symlink eeprom-wrapper ${eeprom_tool})
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${eeprom_tool} DESTINATION ${RUNTIME_DIR})
endforeach(eeprom_tool_source ${eeprom_tool_sources})

install(PROGRAMS
    eeprom-path
    eeprom-wrapper
    eeprom-set-autoboot
    DESTINATION ${RUNTIME_DIR}
)
