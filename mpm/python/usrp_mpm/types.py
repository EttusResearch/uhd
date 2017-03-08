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
from multiprocessing import Value
from multiprocessing import Array
from multiprocessing import Lock
import ctypes

MPM_RPC_PORT = 49601
MPM_DISCOVERY_PORT = 49600

MPM_DISCOVERY_MESSAGE = "MPM-DISC"

class graceful_exit(Exception):
    pass


class shared_state:
    def __init__(self):
        self.lock = Lock()
        self.claim_status = Value(ctypes.c_bool, False, lock=self.lock) # lock
        self.claim_token = Array(ctypes.c_char, 32, lock=self.lock) # String with max length of 32
