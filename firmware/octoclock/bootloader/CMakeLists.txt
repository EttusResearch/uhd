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
set(bootloader_sources
    ../lib/init.c
    ../lib/enc28j60.c
    ../lib/network.c
    ../lib/arp_cache.c
    main.c
)

add_executable(octoclock_bootloader.elf ${bootloader_sources})

#
# The __AVR_LIBC_DEPRECATED_ENABLE__ flag is necessary because of a bug in some versions of avr-libc
# where including <avr/boot.h> when using an atmega128 will cause a compiler error.
#
# Details: http://savannah.nongnu.org/bugs/?36410
#
set_target_properties(octoclock_bootloader.elf PROPERTIES
    COMPILE_FLAGS "${CMAKE_C_FLAGS} -Os -D__AVR_LIBC_DEPRECATED_ENABLE__ -D__BOOTLOADER__ -Wall"
    LINK_FLAGS "-Os -Wl,--relax,--section-start=.text=0x1E000,-Map=octoclock_bootloader.map,--cref"
)


add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/octoclock_bootloader.hex
    DEPENDS octoclock_bootloader.elf
    COMMENT "Generating octoclock_bootloader.hex"
    COMMAND ${AVR_OBJCOPY} -O ihex ${CMAKE_CURRENT_BINARY_DIR}/octoclock_bootloader.elf ${CMAKE_BINARY_DIR}/octoclock_bootloader.hex
)
add_custom_target(
    octoclock_bootloader_hex ALL
    DEPENDS ${CMAKE_BINARY_DIR}/octoclock_bootloader.hex
)

add_custom_target(
    upload_bootloader
    ${AVRDUDE} -p atmega128 -c ${PROGRAMMER} -P usb -U efuse:w:0xFF:m -U hfuse:w:0x80:m -U lfuse:w:0xEF:m -U flash:w:octoclock_bootloader.hex:i
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS octoclock_bootloader_hex
    COMMENT "Uploading OctoClock bootloader to device with ${PROGRAMMER}"
)
