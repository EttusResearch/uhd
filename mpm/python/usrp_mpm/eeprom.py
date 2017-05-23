#
# Copyright 2017 Ettus Research (National Instruments)
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
"""
EEPROM management code
"""

import struct

EEPROM_DEFAULT_HEADER = struct.Struct("!I I")

class MboardEEPROM(object):
    """
    Given a nvmem path, read out EEPROM values from the motherboard's EEPROM.
    The format of data in the EEPROM must follow the following standard:

    - 4 bytes magic. This will always be the same value; checking this value is
                     a sanity check for if the read was successful.
    - 4 bytes version. This is the version of the EEPROM format.

    The following bytes are version-dependent:

    Version 1:

    - 4x4 bytes mcu_flags -> throw them away
    - 2 bytes hw_pid
    - 2 bytes hw_rev (starting at 0)
    - 8 bytes serial number (zero-terminated string of 7 characters)
    - 6 bytes MAC address for eth0
    - 2 bytes padding
    - 6 bytes MAC address for eth1
    - 2 bytes padding
    - 6 bytes MAC address for eth2
    - 2 bytes padding
    - 4 bytes CRC

    MAC addresses are ignored here; they are read elsewhere. If we really need
    to know the MAC address of an interface, we can fish it out the raw data,
    or ask the system.
    """
    # Create one of these for every version of the EEPROM format:
    eeprom_header_format = (
        None, # For laziness, we start at version 1 and thus index 0 stays empty
        "!I I 16x H H 7s 25x I", # Version 1
    )
    eeprom_header_keys = (
        None, # For laziness, we start at version 1 and thus index 0 stays empty
        ('magic', 'eeprom_version', 'pid', 'rev', 'serial', 'CRC'), # Version 1
    )

class DboardEEPROM(object):
    """
    Given a nvmem path, read out EEPROM values from the daughterboard's EEPROM.
    The format of data in the EEPROM must follow the following standard:

    - 4 bytes magic. This will always be the same value; checking this value is
                     a sanity check for if the read was successful.
    - 4 bytes version. This is the version of the EEPROM format.

    The following bytes are version-dependent:

    Version 1:

    - 2 bytes hw_pid
    - 2 bytes hw_rev (starting at 0)
    - 8 bytes serial number (zero-terminated string of 7 characters)
    - 4 bytes CRC

    MAC addresses are ignored here; they are read elsewhere. If we really need
    to know the MAC address of an interface, we can fish it out the raw data,
    or ask the system.
    """
    # Create one of these for every version of the EEPROM format:
    eeprom_header_format = (
        None, # For laziness, we start at version 1 and thus index 0 stays empty
        "!I I H H 7s 1x I", # Version 1
    )
    eeprom_header_keys = (
        None, # For laziness, we start at version 1 and thus index 0 stays empty
        ('magic', 'eeprom_version', 'pid', 'rev', 'serial', 'CRC'), # Version 1
    )


def read_eeprom(
        nvmem_path,
        eeprom_header_format,
        eeprom_header_keys,
        expected_magic,
        max_size=None
):
    """
    Read the EEPROM located at nvmem_path and return a tuple (header, data)
    Header is already parsed in the common header fields
    Data contains the full eeprom data structure

    nvmem_path -- Path to readable file (typically something in sysfs)
    eeprom_header_format -- List of header formats, by version
    eeprom_header_keys -- List of keys for the entries in the EEPROM
    expected_magic -- The magic value that is expected
    max_size -- Max number of bytes to be read. If omitted, will read the full file.
    """
    assert len(eeprom_header_format) == len(eeprom_header_keys)
    def _parse_eeprom_data(
            data,
            version,
        ):
        """
        Parses the raw 'data' according to the version.
        Returns a dictionary.
        """
        eeprom_parser = struct.Struct(eeprom_header_format[version])
        eeprom_keys = eeprom_header_keys[version]
        parsed_data = eeprom_parser.unpack_from(data)
        return dict(zip(eeprom_keys, parsed_data))
    # Dawaj, dawaj
    max_size = max_size or -1
    with open(nvmem_path, "rb") as nvmem_file:
        data = nvmem_file.read(max_size)
    eeprom_magic, eeprom_version = EEPROM_DEFAULT_HEADER.unpack_from(data)
    if eeprom_magic != expected_magic:
        raise RuntimeError(
            "Received incorrect EEPROM magic. " \
            "Read: {:08X} Expected: {:08X}".format(
               eeprom_magic, expected_magic
       ))
    if eeprom_version >= len(eeprom_header_format):
        raise RuntimeError("Unexpected EEPROM version: `{}'".format(eeprom_version))
    return (_parse_eeprom_data(data, eeprom_version), data)


