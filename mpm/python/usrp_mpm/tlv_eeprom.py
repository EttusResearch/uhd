#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Tag-Length-Value (TLV) based EEPROM management code
"""

import struct
import zlib

class NamedStruct:
    """
    Helper class for unpacking values from bytes
    """
    def __init__(self, fmt, keys):
        self.struct = struct.Struct(fmt)
        self.keys = keys

        # ensure no duplicate keys
        assert len(set(keys) - set([None])) == \
            len([x for x in keys if x is not None])
        # ensure same number of keys as elements
        assert len(self.struct.unpack(bytearray(self.size))) == len(keys)

    def unpack_from(self, buf, offset=0):
        """
        Unpack values from the struct, returning a dictionary mapping a field
        name to a value. If the field name is None, the field is not included
        in the returned dictionary.

        buf -- the buffer to unpack from
        offset -- the offset within that buffer
        """

        vals = self.struct.unpack_from(buf, offset)
        return {x[0]: x[1] for x in zip(self.keys, vals) if x[0] is not None}

    @property
    def size(self):
        """
        number of values to unpack
        """
        return self.struct.size


def tlv_eeprom_validate(eeprom, expected_magic):
    """
    Validate the contents of the EEPROM and return a tuple (header, tlv)
    Header is a dictionary of the EEPROM header (magic, size, crc)
    Tlv is the raw TLV data

    eeprom -- raw eeprom data
    expected_magic -- magic value that's expected
    """
    def crc32(data, initial=0):
        initial = initial ^ 0xFFFFFFFF
        crc = zlib.crc32(data, initial)
        return crc ^ 0xFFFFFFFF

    size_offset = 8
    tlv_eeprom_hdr = NamedStruct('< I I I', ['magic', 'crc', 'size'])
    hdr = tlv_eeprom_hdr.unpack_from(eeprom)

    if hdr['magic'] != expected_magic:
        raise RuntimeError(
            "Received incorrect EEPROM magic. "
            "Read: {:08X} Expected: {:08X}".format(
                hdr['magic'], expected_magic))

    if hdr['size'] > (len(eeprom) - tlv_eeprom_hdr.size):
        raise RuntimeError('invalid size')

    crc = crc32(eeprom[size_offset:tlv_eeprom_hdr.size+hdr['size']])
    if hdr['crc'] != crc:
        raise RuntimeError(
            "Received incorrect CRC. "
            "Read: {:08X} Expected: {:08X}".format(
                hdr['crc'], crc))

    return hdr, eeprom[tlv_eeprom_hdr.size:tlv_eeprom_hdr.size+hdr['size']]


def tlv_eeprom_unpack(tlv, tagmap):
    """
    Parse TLV data and return a dictionary of values found

    tlv -- raw TLV data from the EEPROM
    tagmap -- dictionary mapping 8-bit tag to a NamedStruct instance
    """

    values = {}
    hdr_struct = NamedStruct('< B B', ['tag', 'len'])
    idx = 0

    while idx < len(tlv):
        hdr = hdr_struct.unpack_from(tlv, idx)
        idx += hdr_struct.size

        if hdr['tag'] in tagmap:
            val_struct = tagmap[hdr['tag']]
            if hdr['len'] != val_struct.size:
                raise RuntimeError(
                    "unexpected size: {:d}, expected: {:d}".format(
                        hdr['len'], tagmap[hdr['tag']]))
            unpacked = val_struct.unpack_from(tlv, idx)
            # prohibit clobbering existing values
            assert len(set(values.keys()) & set(unpacked.keys())) == 0
            values.update(unpacked)

        idx += hdr['len']

    return values


def read_eeprom(
        nvmem_path,
        tagmap,
        expected_magic,
        max_size=None
):
    """
    Read the EEPROM located at nvmem_path and return a tuple (header, data)
    Header is a dictionary of values unpacked from eeprom based upon tagmap
    Data contains the full eeprom contents

    nvmem_path -- Path to readable file (typically something in sysfs)
    tagmap -- dictionary mapping an 8-bit tag to a NamedStruct instance
    expected_magic -- magic value that is expected
    max_size -- Max number of bytes to read from nvmem, whole file if omitted

    Tagmap should be a dictionary mapping an 8-bit tag to a NamedStruct
    instance. For each tag that's found in the eeprom, the value at that tag
    will be unpacked using the associated NamedStruct; any named fields within
    that struct will then be added to the returned dictionary.
    """

    max_size = max_size or -1
    with open(nvmem_path, "rb") as nvmem_file:
        data = nvmem_file.read(max_size)
    _, tlv = tlv_eeprom_validate(data, expected_magic)
    return tlv_eeprom_unpack(tlv, tagmap), data
