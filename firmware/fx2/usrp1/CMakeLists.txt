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
set(libusrp1_sources 
    ${CMAKE_SOURCE_DIR}/common/delay.c
    ${CMAKE_SOURCE_DIR}/common/fx2utils.c
    ${CMAKE_SOURCE_DIR}/common/i2c.c
    ${CMAKE_SOURCE_DIR}/common/init_gpif.c
    ${CMAKE_SOURCE_DIR}/common/isr.c
    ${CMAKE_SOURCE_DIR}/common/timer.c
    ${CMAKE_SOURCE_DIR}/common/usb_common.c
#    ${CMAKE_SOURCE_DIR}/common/spi.c
#    ${CMAKE_SOURCE_DIR}/common/vectors.a51
)

#file(GLOB libusrp1_c_sources ${CMAKE_SOURCE_DIR}/common/*.c)
#file(GLOB libusrp1_a51_sources ${CMAKE_SOURCE_DIR}/common/*.a51)
#list(APPEND libusrp1_sources ${libusrp1_c_sources} ${libusrp1_a51_sources})

add_library(libusrp1 STATIC ${libusrp1_sources})

# edit-gpif hacks up gpif.c for our purposes. no major surgery, just moving stuff around.
set(GPIF_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/gpif.c)
set(GPIF_SOURCE_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/usrp_gpif.c)
set(GPIF_HEADER_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/usrp_gpif_inline.h)

add_custom_command(
        OUTPUT ${GPIF_SOURCE_OUTPUT}
        DEPENDS ${EDIT_GPIF_USRP1}
        COMMAND ${PYTHON_EXECUTABLE} ${EDIT_GPIF_USRP1} ${GPIF_SOURCE} ${GPIF_SOURCE_OUTPUT} ${GPIF_HEADER_OUTPUT}
        COMMENT "Generating ${GPIF_SOURCE_OUTPUT}"
)

#file(GLOB usrp1_sources *.c)
set(usrp1_sources 
    ${CMAKE_SOURCE_DIR}/common/vectors.a51
    board_specific.c 
    eeprom_io.c 
    fpga_load.c 
    fpga_rev2.c 
    usrp_common.c 
    usrp_gpif.c 
    usrp_main.c 
    usb_descriptors.a51 
    ${CMAKE_SOURCE_DIR}/common/spi.c
    ${CMAKE_SOURCE_DIR}/common/_startup.a51
)
add_executable(usrp1_fw ${usrp1_sources})
target_link_libraries(usrp1_fw libusrp1)

set(eeprom1_sources
    ${CMAKE_SOURCE_DIR}/common/eeprom_boot.a51
    ${CMAKE_SOURCE_DIR}/common/eeprom_init.c
    ${CMAKE_SOURCE_DIR}/common/_startup.a51
)

add_custom_target(usrp1_eeprom ALL
    DEPENDS usrp1_boot
    COMMAND objcopy -I ihex -O binary usrp1_boot.ihx usrp1_boot.bin
    COMMAND ${PYTHON_EXECUTABLE} ${BUILD_EEPROM} -r1 usrp1_boot.bin usrp1_eeprom.bin
)

add_custom_target(usrp1_eeprom_header ALL
    DEPENDS usrp1_eeprom
    COMMAND xxd -i usrp1_eeprom.bin ${CMAKE_SOURCE_DIR}/../../host/utils/usrp1_eeprom.h
    COMMAND sed -i 's/char/const char/' ${CMAKE_SOURCE_DIR}/../../host/utils/usrp1_eeprom.h
)

add_executable(usrp1_boot ${eeprom1_sources})
target_link_libraries(usrp1_boot libusrp1)
