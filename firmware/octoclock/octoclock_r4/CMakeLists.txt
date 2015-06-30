#
# Copyright 2014 Ettus Research LLC
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

add_executable(octoclock_r4_fw.elf octoclock_r4_main.c)
target_link_libraries(octoclock_r4_fw.elf octoclock)
set_target_properties(octoclock_r4_fw.elf PROPERTIES
    LINK_FLAGS "-Wl,--relax,-Map=octoclock_r4_fw.map,--cref"
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/octoclock_r4_fw.bin
    DEPENDS octoclock_r4_fw.elf
    COMMENT "Generating octoclock_r4_fw.bin"
    COMMAND ${AVR_OBJCOPY} -O binary ${CMAKE_CURRENT_BINARY_DIR}/octoclock_r4_fw.elf ${CMAKE_BINARY_DIR}/octoclock_r4_fw.bin
)
add_custom_target(
    octoclock_r4_fw_bin ALL
    DEPENDS ${CMAKE_BINARY_DIR}/octoclock_r4_fw.bin
)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/octoclock_r4_fw.hex
    DEPENDS octoclock_r4_fw.elf
    COMMENT "Generating octoclock_r4_fw.hex"
    COMMAND ${AVR_OBJCOPY} -O ihex ${CMAKE_CURRENT_BINARY_DIR}/octoclock_r4_fw.elf ${CMAKE_BINARY_DIR}/octoclock_r4_fw.hex
)
add_custom_target(
    octoclock_r4_fw_hex ALL
    DEPENDS ${CMAKE_BINARY_DIR}/octoclock_r4_fw.hex
)
add_custom_target(
    upload_r4
    ${AVRDUDE} -p atmega128 -c ${PROGRAMMER} -P usb -U efuse:w:0xFF:m -U hfuse:w:0x81:m -U lfuse:w:0xEF:m -U flash:w:octoclock_r4_fw.hex:i
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS octoclock_r4_fw_hex
    COMMENT "Uploading OctoClock firmware to device with ${PROGRAMMER}"
)
