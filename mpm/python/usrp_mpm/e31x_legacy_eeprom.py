#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
E310 EEPROM management code
"""

import struct
from builtins import zip
from builtins import object

# pylint: disable=too-few-public-methods
class MboardEEPROM(object):
    """
    Given a nvmem path, read out EEPROM values from the motherboard's EEPROM.
    The format of data in the EEPROM must follow the following standard:

    E310 Legacy EEPROM Format

    - 2 bytes data_version_major
    - 2 bytes data_version_minor
    - 6 bytes MAC address
    - 2 bytes hw_pid
    - 2 bytes hw_rev
    - 8 bytes serial number (xFF or NULL terminated)
    - 12 bytes padding
    - 8 bytes user_name

    MAC addresses are ignored here; they are read elsewhere. If we really need
    to know the MAC address of an interface, we can fish it out the raw data,
    or ask the system.
    """
    # Refer e300_eeprom_manager.hpp.
    eeprom_header_format = "<H H 6s H H 8s 12s 8s"
    eeprom_header_keys = (
        'data_version_major',
        'data_version_minor',
        'mac_address',
        'pid',
        'rev',
        'serial',
        'pad',
        'user_name')

class DboardEEPROM(object):
    """
    Given a nvmem path, read out EEPROM values from the daughterboard's EEPROM.
    The format of data in the EEPROM must follow the following standard:

    E310 Legacy EEPROM Format

    - 2 bytes data_version_major
    - 2 bytes data_version_minor
    - 2 bytes hw_pid
    - 2 bytes hw_rev
    - 8 bytes serial number (xFF or NULL terminated)
    - 12 bytes padding
    """
    # Refer e300_eeprom_manager.hpp.
    eeprom_header_format = "<H H H H 8s 12s"
    eeprom_header_keys = (
        'data_version_major',
        'data_version_minor',
        'pid',
        'rev',
        'serial',
        'pad')
# pylint: disable=too-few-public-methods

def read_eeprom(
        is_motherboard,
        nvmem_path,
        offset,
        eeprom_header_format,
        eeprom_header_keys,
        max_size=None,
):
    """
    Read the EEPROM located at nvmem_path and return a tuple (header, data)
    Header is already parsed in the common header fields
    Data contains the full eeprom data structure

    nvmem_path -- Path to readable file (typically something in sysfs)
    eeprom_header_format -- List of header formats, by version
    eeprom_header_keys -- List of keys for the entries in the EEPROM
    max_size -- Max number of bytes to be read. If omitted, will read the full file.
    """
    max_size = max_size or -1
    with open(nvmem_path, "rb") as nvmem_file:
        data = nvmem_file.read(max_size)[offset:]
    eeprom_parser = struct.Struct(eeprom_header_format)
    eeprom_keys = eeprom_header_keys
    parsed_data = eeprom_parser.unpack_from(data)

    if is_motherboard: # E310 MB.
        # Rectify the PID and REV parsing. Reverse the bytes.
        # PID and REV are the 4th and 5th elements in the tuple.
        parsed_data_list = list(parsed_data)
        parsed_data_list[3] = struct.unpack("<H", struct.pack(">H", parsed_data_list[3]))[0]
        parsed_data_list[4] = struct.unpack("<H", struct.pack(">H", parsed_data_list[4]))[0]
        # Some revisions use xFF terminated strings for serial and user_name.
        # Replace xFF with NULL to pass ascii conversion.
        parsed_data_list[5] = parsed_data_list[5].replace(b'\xff',b'\x00')
        parsed_data_list[7] = parsed_data_list[7].replace(b'\xff',b'\x00')
        parsed_data = tuple(parsed_data_list)

    else: # E310 DB.
        # Rectify the PID and REV parsing. Reverse the bytes.
        # PID and REV are the 3rd and 4th elements in the tuple.
        parsed_data_list = list(parsed_data)
        parsed_data_list[2] = struct.unpack("<H", struct.pack(">H", parsed_data_list[2]))[0]
        parsed_data_list[3] = struct.unpack("<H", struct.pack(">H", parsed_data_list[3]))[0]
        # Some revisions use xFF terminated strings for serial.
        # Replace xFF with NULL to pass ascii conversion.
        parsed_data_list[4] = parsed_data_list[4].replace(b'\xff',b'\x00')
        parsed_data = tuple(parsed_data_list)

    ret_val = (dict(list(zip(eeprom_keys, parsed_data))), data)
    return ret_val
