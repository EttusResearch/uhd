#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
BufferFS. Serialization mini-library meant for use with EEPROMs.
"""

from __future__ import print_function, division
import copy
import struct
import zlib
from collections import OrderedDict
from builtins import str
from six import itervalues

DEFAULT_ALIGNMENT = 1024 # bytes

def align_addr(addr, align_to):
    """
    Align an address to an alignment boundary, rounding upwards.

    >>> align_addr(5, 8)
    8
    >>> align_addr(21, 8)
    24
    >>> align_addr(16, 16)
    16
    """
    div, mod = divmod(addr, align_to)
    return (div + (1 if mod else 0)) * align_to

def _normalize_byte_str(identifier, length=8, pad=b'\0'):
    " Guarantees that identifier is byte-string of length 'length' "
    identifier = bytes(identifier, 'ascii')
    if len(identifier) < length:
        identifier = identifier + pad * (length - len(identifier))
    return identifier[:length]

class BufferFS(object):
    """
    Buffer-FS -- Map dictionaries of arbitrary byte strings into a buffer.

    This can be useful for storing arbitrary blobs in EEPROMs, but the
    implementation is not specific to EEPROMS.

    Effectively, this is a serialization class with some CRC checking and byte-
    alignment. Something like pickle will often also do the trick.

    Arguments:
    raw_data_buffer -- A copy of the original buffer
    max_size -- Max length of the buffer in bytes. raw_data_buffer may be
                smaller than this.
    alignment -- This will align blobs to certain address boundaries.
    log -- Logger object. If none is given, one will be created.
    """
    magic = b'TofC'
    default_header = ("!4s I", ('magic', 'version'))
    default_version = 0

    # Version 0 TOC:
    # --------------
    # 4 bytes magic number
    # 4 bytes magic version (note: Up to here, all versions of the TOC will
    #                        look the same)
    # 4 bytes num entries
    # <entries>
    # 4 bytes CRC of entire TOC
    #
    # Version 0 Entry:
    # --------------
    # 4 bytes base address
    # 4 bytes length in bytes
    # 4 bytes CRC of entry
    # 8 bytes identifier (string, can use nulls to be shorter than 8 bytes)
    entry = {
        0: ("!I I I 8s", ('base', 'length', 'CRC', 'id')),
    }


    def __init__(self, raw_data_buffer, max_size=None, alignment=None, log=None):
        assert isinstance(raw_data_buffer, bytes)
        self.max_size = len(raw_data_buffer) if max_size is None else max_size
        self.raw_data_buffer = raw_data_buffer[:self.max_size]
        self.alignment = alignment or DEFAULT_ALIGNMENT
        self.pad = b'\xFF'
        if log is None:
            from usrp_mpm import mpmlog
            self.log = mpmlog.get_main_logger().getChild("EEPROMFS")
        else:
            self.log = log
        header = self._parse_header(raw_data_buffer)
        self.log.trace("EEPROM-FS header has {} valid entries.".format(
            len(header.get('entries', []))))
        self.entries = OrderedDict({
            str(x['id'], encoding='ascii'): x for x in header.get('entries', [])
        })
        self.buffer = self._trunc_buffer(raw_data_buffer, self.entries)
        self.log.trace("Truncated buffer to length %d", len(self.buffer))
        # Start storing entries at 128
        self.entries_base = 128
        # We can only store so many entries before running out of TOC space
        self.max_entries = (128 - 16) // 20
        # TODO -- these last two shouldn't be hard coded
        self.log.trace(
            "This BufferFS has {} max entries, starting at {}".format(
                self.max_entries, self.entries_base))

    def _parse_header(self, buf):
        """
        Read the buffer and return header info as a list of dictionaries.
        """
        default_hdr_struct = struct.Struct(self.default_header[0])
        if len(buf) < default_hdr_struct.size + 8:
            return {}
        default_hdr_unpacked = default_hdr_struct.unpack_from(buf)
        hdr = dict(list(zip(self.default_header[1], default_hdr_unpacked)))
        # There might be no EEPROM-FS, that's legit:
        if hdr['magic'] != self.magic:
            self.log.trace(
                "No Buffer-FS magic found (was: `{}'), " \
                "returning empty dict.".format(hdr['magic']))
            return {}
        self.log.trace("Buffer has correct magic word...")
        # The rest of this function assumes version is 0:
        toc_version = int(hdr['version'])
        self.log.trace("Found ToC version: 0x{}".format(toc_version))
        assert toc_version == 0
        num_entries_struct = struct.Struct('!I')
        num_entries = num_entries_struct.unpack_from(
            buf, offset=default_hdr_struct.size
        )[0]
        self.log.trace("Header declares num entries: {}".format(num_entries))
        toc_offset = default_hdr_struct.size + num_entries_struct.size
        self.log.trace("TOC offset: {}".format(toc_offset))
        entry_struct = struct.Struct(self.entry[toc_version][0])
        entries = []
        for entry_idx in range(num_entries):
            entry_offset = toc_offset+entry_idx*entry_struct.size
            entry_unpacked = \
                entry_struct.unpack_from(
                    buf,
                    offset=entry_offset
                )
            entries.append(
                dict(list(zip(self.entry[toc_version][1], entry_unpacked)))
            )
            entries[-1]['id'] = entries[-1]['id'].rstrip(b'\0')
        self.log.trace("TOC has %d entries (CRC un-checked)", len(entries))
        crc_offset = toc_offset + num_entries * entry_struct.size
        self.log.trace("TOC CRC offset: %d", crc_offset)
        crc_struct = struct.Struct('!I')
        crc = crc_struct.unpack_from(buf, offset=crc_offset)[0]
        self.log.trace("Calculating TOC CRC32 on %d bytes...", crc_offset)
        expected_crc = zlib.crc32(buf[:crc_offset])
        if crc != expected_crc:
            self.log.warning(
                "EEPROM-FS Header CRC failed! " \
                "Read: {:08X} Expected: {:08X}".format(crc, expected_crc))
            return hdr
        self.log.trace("CRC32 matches.")
        hdr['entries'] = entries
        return hdr

    def _trunc_buffer(self, buf, entries):
        """
        Return the shortest possible buf that contains all entries.
        """
        max_len = max([
            x['base'] + x['length'] for x in itervalues(entries)
        ] + [0])
        return buf[:max_len]


    def get_blob(self, identifier, entries=None, buf=None):
        """
        Return blob by ID.

        By default, will use the internal entries table and buffer.
        """
        entries = entries or self.entries
        buf = buf or self.buffer
        if identifier not in entries:
            raise RuntimeError("Requesting non-existent blob {}!".format(
                identifier))
        entry_info = entries[identifier]
        entry_base = entry_info['base']
        entry_len = entry_info['length']
        entry_buf = buf[entry_base:entry_base+entry_len]
        entry_crc = zlib.crc32(entry_buf)
        self.log.trace("Calculating blob CRC32 on %d bytes...", len(entry_buf))
        if entry_crc != entry_info['CRC']:
            raise RuntimeError(
                "Entry `{}' has CRC mismatch! " \
                "Calculated {:08X}, expected {:08X}.".format(
                    identifier, entry_crc, entry_info['CRC']
                )
            )
        return entry_buf

    def has_blob(self, identifier):
        """
        Returns True if the blob 'identifier' exists.
        """
        return self.entries.has_key(identifier)

    def set_blob(self, identifier, blob):
        """
        Add a blob to the list.
        """
        self.log.trace("Attempting to add new blob `{}'...".format(identifier))
        identifier = _normalize_byte_str(identifier, length=8)
        identifier_str = str(identifier.rstrip(b'\0'), encoding='ascii')
        if identifier_str not in self.entries and \
                len(self.entries) >= self.max_entries:
            self.log.error("Exceeded entry limit.")
            raise RuntimeError("Exceeded entry limit.")
        entry_info = {
            'CRC': zlib.crc32(blob),
            'length': len(blob),
            'id': identifier,
        }
        alignment = self.alignment
        self.log.trace("Byte-alignment is {}".format(alignment))
        new_entries = copy.copy(self.entries)
        entry_base = self._find_base(entry_info, new_entries,
                                     alignment=alignment)
        self.log.trace("First attempt at finding a base yields: {}".format(
            entry_base
        ))
        new_entries.pop(identifier, None)
        if entry_base is None:
            self.log.trace("First attempt to find a spot failed.")
            space_occupied = self._calc_space_occupied(
                new_entries,
                alignment=alignment
            )
            self.log.trace("Current blobs are occupying {} bytes.".format(
                space_occupied
            ))
            if space_occupied + entry_info['length'] > self.max_size:
                raise RuntimeError("Not enough space to store blob!")
            new_entries, new_buffer = \
                    self._pack_entries(new_entries, self.buffer, alignment)
            entry_base = self._find_base(
                entry_info,
                new_entries,
                alignment=alignment
            )
            self.log.trace("2nd attempt at finding a base yields: {}".format(
                entry_base
            ))
            if entry_base is None:
                raise RuntimeError("Unexpected failure trying to park new blob!")
            self.buffer, self.entries = new_buffer, new_entries
        entry_info['base'] = entry_base
        if len(self.buffer) < entry_base:
            self.buffer += self.pad * (entry_base - len(self.buffer))
        assert len(self.buffer) >= entry_base
        self.entries[identifier_str] = entry_info
        buf_base = \
            self.buffer[:self.entries_base] + \
            self.pad * (self.entries_base - len(self.buffer[:self.entries_base]))
        assert len(buf_base) == self.entries_base
        self.log.trace("Updating TOC...")
        buf_base = self._update_toc(self.entries, buf_base)
        self.log.trace("Splicing new blob into buffer...")
        assert len(buf_base) == self.entries_base
        self.buffer = self._trunc_buffer(
            buf_base \
                + self.buffer[len(buf_base):entry_base] \
                + blob \
                + self.buffer[entry_base+entry_info['length']:],
            self.entries,
        )


    def _find_base(self, new_entry, entries, alignment):
        """
        Find a spot to park a new entry.

        If it's actually the same ID as an existing entry, try and re-use that
        space. If the previous entry was smaller, and there's another entry
        following, move the entry towards the end.

        If it can't overwrite an existing entry, or append (because of space
        limitations), don't try and be smart. Just return None.
        """
        entry_id = str(new_entry['id'].rstrip(b'\0'), encoding='ascii')
        entry_len = new_entry['length']
        self.log.trace(
            "Trying to find a spot for blob `%s' of length %d",
            entry_id, entry_len
        )
        if entry_id in entries and \
                (entry_len <= entries[entry_id]['length'] or \
                 entries[entry_id]['base'] == \
                    max((x['base'] for x in itervalues(entries)))
                ):
            self.log.trace(
                "Blob was already in index, reusing address %d",
                entries[entry_id]['base'],
            )
            return entries[entry_id]['base']
        last_base = \
            max([x['base'] + x['length'] for x in itervalues(entries)] \
                + [self.entries_base])
        self.log.trace("New entry needs to go after address %d", last_base)
        new_base = align_addr(last_base, alignment)
        self.log.trace("New address is: %d (Alignment is: %d)",
                       new_base, alignment)
        if new_base + entry_len < self.max_size:
            return new_base
        self.log.debug(
            "New base address %d and length %d would exceed EEPROM size",
            new_base, entry_len
        )
        return None

    def _calc_space_occupied(self, entries, alignment):
        """
        Returns the number of bytes required to store TOC and entries, given
        a certain alignment.
        """
        return sum(
            [align_addr(x['length'], alignment) for x in itervalues(entries)],
            align_addr(self.entries_base, alignment),
        )

    def _pack_entries(self, entries_, buf, alignment):
        """
        Reorder entries to minimize fragmentation, then return a new buf

        Note: This is not going to try and be smart. In whatever order the
        blobs are stored, they will stay in that order. Reordering could be
        better given a certain alignment, but that's "room for improvement".
        """
        raise NotImplementedError("tbi") # FIXME
        # Algorithm is fairly simple:
        # - Copy all entries_ into a new dict entries
        # entries = copy.copy(entries_)
        # - Read all blobs from buf, make another dictionary id -> blob,
        #   storing all the blobs
        # - Go through the entries in order, recalculate base addresses such
        #   that they are maximally packed.
        #   First address is self.entries_base, second base address is
        #   align_addr(first_entry_base + len(first_blob)), third address is
        #   align_addr(second_entry_base + len(second_blob)), and so on
        # - Then, create a string that consists of a new TOC, and all the blobs
        #   with appropriate padding

    def _update_toc(self, entries, toc_buf):
        """
        Returns a new TOC buffer based on entries.
        """
        toc_version = 0 # This method is hardcoded to version 0
                        # Not a great example of generic SW design
        entries_sorted = sorted(entries.values(), key=lambda x: x['base'])
        new_toc = \
                struct.Struct(self.default_header[0]).pack(self.magic, 0) + \
                struct.Struct('!I').pack(len(entries))
        entry_struct = struct.Struct(self.entry[toc_version][0])
        for entry_info in entries_sorted:
            new_toc += entry_struct.pack(
                entry_info['base'],
                entry_info['length'],
                entry_info['CRC'],
                entry_info['id'],
            )
        self.log.trace("Calculating new TOC CRC32 on %d bytes...", len(new_toc))
        new_toc_crc = zlib.crc32(new_toc)
        new_toc += struct.Struct('!I').pack(new_toc_crc)
        assert len(new_toc) < self.entries_base
        return new_toc + toc_buf[len(new_toc):]

