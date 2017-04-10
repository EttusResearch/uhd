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
Access to UIO mapped memory.
"""

import struct
import os
import mmap

class uio(object):
    """
    Provides peek/poke interfaces for uio-mapped memory.

    Arguments:
    path -- Path to UIO device, e.g. '/dev/uio0'
    length -- Number of bytes in the address space (is passed to mmap.mmap).
              Must be at least one page size
    read_only -- Boolean, True == ro, False == rw
    offset -- Passed to mmap.mmap
    """
    def __init__(self, path, length, read_only=True, offset=None):
        # Python can't tell the size of a uio device
        assert length >= mmap.PAGESIZE
        offset = offset or 0
        self._path = path
        self._read_only = read_only
        self._fd = os.open(path, os.O_RDONLY if read_only else os.O_RDWR)
        self._mm = mmap.mmap(
            self._fd,
            length,
            flags=mmap.MAP_SHARED,
            prot=mmap.PROT_READ | (0 if read_only else mmap.PROT_WRITE),
            offset=offset,
        )

    def peek32(self, addr):
        """
        Returns the 32-bit value starting at address addr as an integer
        """
        return struct.unpack('@I', self._mm[addr:addr+4])[0]

    def poke32(self, addr, val):
        """
        Writes the 32-bit value val to address starting at addr.
        Will throw if read_only was set to True.
        A value that exceeds 32 bits will be truncated to 32 bits.
        """
        assert not self._read_only
        self._mm[addr:addr+4] = struct.pack(
            '@I',
            (val & 0xFFFFFFFF),
        )

