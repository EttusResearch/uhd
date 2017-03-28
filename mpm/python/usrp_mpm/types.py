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


class EEPROM(object):
    """
    Reads out common properties and rawdata out of a nvmem path
    """
    # eeprom_header contains:
    # 4 bytes magic
    # 4 bytes CRC
    # 2 bytes data_version
    # 2 bytes hw_pid
    # 2 bytes hw_rev
    # 2 bytes pad
    #
    # 16 bytes in total
    eeprom_header = struct.Struct("I I H H H 2x")

    def read_eeprom(self, nvmem_path):
        """
        Read the EEPROM located at nvmem_path and return a tuple (header, body)
        Header is already parsed in the common header fields
        """
        with open(nvmem_path, "rb") as nvmem_file:
            header = nvmem_file.read(16)
            data = nvmem_file.read(240)
        header = self.eeprom_header.unpack(header)
        header = {
            "magic": header[0],
            "crc": header[1],
            "data_version": header[2],
            "hw_pid": header[3],
            "hw_rev": header[4],
        }
        return (header, data)
