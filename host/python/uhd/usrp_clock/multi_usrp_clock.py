#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" @package usrp_clock
Python UHD module containing the MultiUSRPClock
"""

from .. import libpyuhd as lib


class MultiUSRPClock(lib.usrp_clock.multi_usrp_clock):
    """
    MultiUSRPClock object for controlling devices
    """
    def __init__(self, args=""):
        """MultiUSRPClock constructor"""
        super(MultiUSRPClock, self).__init__(args)

