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

include_directories(${CMAKE_SOURCE_DIR}/common)

#for usrp_common.h and the regs files...
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#now make a lib to link against
set(libb100_sources 
    ${CMAKE_SOURCE_DIR}/common/delay.c
    ${CMAKE_SOURCE_DIR}/common/fx2utils.c
    ${CMAKE_SOURCE_DIR}/common/i2c.c
    ${CMAKE_SOURCE_DIR}/common/init_gpif.c
    ${CMAKE_SOURCE_DIR}/common/isr.c
    ${CMAKE_SOURCE_DIR}/common/timer.c
    ${CMAKE_SOURCE_DIR}/common/usb_common.c
#    ${CMAKE_SOURCE_DIR}/common/spi.c
)

#file(GLOB libb100_c_sources ${CMAKE_SOURCE_DIR}/common/*.c)
#file(GLOB libb100_a51_sources ${CMAKE_SOURCE_DIR}/common/*.a51)
#list(APPEND libb100_sources ${libb100_c_sources} ${libb100_a51_sources})

add_library(libb100 STATIC ${libb100_sources})

# edit-gpif hacks up gpif.c for our purposes. no major surgery, just moving stuff around.
set(GPIF_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/gpif.c)
set(GPIF_SOURCE_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/usrp_gpif.c)
set(GPIF_HEADER_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/usrp_gpif_inline.h)

add_custom_command(
        OUTPUT ${GPIF_SOURCE_OUTPUT}
        DEPENDS ${EDIT_GPIF_B100}
        COMMAND ${PYTHON_EXECUTABLE} ${EDIT_GPIF_B100} ${GPIF_SOURCE} ${GPIF_SOURCE_OUTPUT} ${GPIF_HEADER_OUTPUT}
        COMMENT "Generating ${GPIF_SOURCE_OUTPUT}"
)

#file(GLOB b100_sources *.c)
set(b100_sources 
    ${CMAKE_SOURCE_DIR}/common/vectors.a51
    usrp_main.c
    usrp_common.c 
    board_specific.c 
    fpga_load.c 
    fpga_rev2.c 
    usrp_gpif.c 
    usb_descriptors.a51
    eeprom_io.c
    ${CMAKE_SOURCE_DIR}/common/_startup.a51
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/usrp_main.c
    PROPERTIES COMPILE_FLAGS "--std-sdcc99 --opt-code-speed --fommit-frame-pointer"
)

add_executable(b100_fw ${b100_sources})
target_link_libraries(b100_fw libb100)

set(eeprom1p_sources
    ${CMAKE_SOURCE_DIR}/common/eeprom_boot.a51
    ${CMAKE_SOURCE_DIR}/common/eeprom_init.c
    ${CMAKE_SOURCE_DIR}/common/_startup.a51
)

add_custom_target(b100_eeprom ALL
    DEPENDS b100_boot
    COMMAND objcopy -I ihex -O binary b100_boot.ihx b100_boot.bin
    COMMAND ${PYTHON_EXECUTABLE} ${BUILD_EEPROM} -r2 b100_boot.bin b100_eeprom.bin
)

add_custom_target(b100_eeprom_header ALL
    DEPENDS b100_eeprom
    COMMAND xxd -i b100_eeprom.bin ${CMAKE_SOURCE_DIR}/../../host/utils/b100_eeprom.h
    COMMAND sed -i 's/char/const char/' ${CMAKE_SOURCE_DIR}/../../host/utils/b100_eeprom.h
)

add_executable(b100_boot ${eeprom1p_sources})
target_link_libraries(b100_boot libb100)
