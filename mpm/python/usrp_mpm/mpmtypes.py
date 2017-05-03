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
MPM types
"""
import ctypes
from multiprocessing import Value
from multiprocessing import Array
from multiprocessing import RLock
import struct

MPM_RPC_PORT = 49601
MPM_DISCOVERY_PORT = 49600

MPM_DISCOVERY_MESSAGE = "MPM-DISC"


class SharedState(object):
    """
    Holds information which should be shared between processes
    Usage should be kept to a minimum
    """

    def __init__(self):
        self.lock = RLock()
        self.claim_status = Value(
            ctypes.c_bool,
            False, lock=self.lock)  # lock
        self.claim_token = Array(
            ctypes.c_char, 256,
            lock=self.lock)  # String with max length of 256


class SID(object):
    def __init__(self, sid=0):
        self.src_addr = sid >> 24
        self.src_ep = (sid >> 16) & 0xFF
        self.dst_addr = (sid >> 8) & 0xFF
        self.dst_ep = sid & 0xFF

    def set_src_addr(self, new_addr):
        self.src_addr =  new_addr & 0xFF

    def set_dst_addr(self, new_addr):
        self.dst_addr = new_addr & 0xFF

    def set_src_ep(self, new_addr):
        self.src_ep = new_addr & 0xFF

    def set_dst_ep(self, new_addr):
        self.dst_ep = new_addr & 0xFF

    def get(self):
        return (self.src_addr << 24) | (self.src_ep << 16) | (self.dst_addr << 8) | self.dst_ep


class EEPROM(object):
    """
    Reads out common properties and rawdata out of a nvmem path
    """
    # eeprom_header contains:
    # 4 bytes magic
    # 4 bytes version
    # 4x4 bytes mcu_flags -> throw them away
    # 2 bytes hw_pid
    # 2 bytes hw_rev
    #
    # 28 bytes in total
    eeprom_header = struct.Struct("!I I 16x H H")

    def read_eeprom(self, nvmem_path):
        """
        Read the EEPROM located at nvmem_path and return a tuple (header, data)
        Header is already parsed in the common header fields
        Data contains the full eeprom data structure
        """
        with open(nvmem_path, "rb") as nvmem_file:
            data = nvmem_file.read(256)
        _header = self.eeprom_header.unpack_from(data)
        print hex(_header[0]), hex(_header[1]), hex(_header[2]), hex(_header[3])
        header = {
            "magic": _header[0],
            "version": _header[1],
            "hw_pid": _header[2],
            "hw_rev": _header[3],
        }
        print header
        return (header, data)
