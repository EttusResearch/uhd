#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
MPM types
"""

import ctypes
from multiprocessing import Value
from multiprocessing import Array
from multiprocessing import RLock

MPM_RPC_PORT = 49601
MPM_DISCOVERY_PORT = 49600
MPM_DISCOVERY_MESSAGE = "MPM-DISC"

# pylint: disable=too-few-public-methods
class SharedState:
    """
    Holds information which should be shared between processes.
    """
    def __init__(self):
        self.lock = RLock()
        self.claim_status = Value(ctypes.c_bool, False, lock=self.lock)
        self.system_ready = Value(ctypes.c_bool, False, lock=self.lock)
        # String with max length of 256:
        self.claim_token = Array(ctypes.c_char, 256, lock=self.lock)
        self.dev_type = Array(ctypes.c_char, 16, lock=self.lock)
        self.dev_serial = Array(ctypes.c_char, 8, lock=self.lock)
        self.dev_product = Array(ctypes.c_char, 16, lock=self.lock)
        self.dev_fpga_type = Array(ctypes.c_char, 8, lock=self.lock)
