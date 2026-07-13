#
# Copyright 2017-2026 Ettus Research, A National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""Utilty functions for the sysfs fpga_manager API."""
import pyudev


class FpgaManager:
    """Class representing the fpga_manager class in sysfs."""

    def __init__(self):
        """Initalize the udev context and find the fpga_manager device."""
        context = pyudev.Context()
        self.fpga_manager = [x for x in context.list_devices(subsystem="fpga_manager")][0]

    def get_state(self):
        """Get the state of the FPGA manager."""
        with open(self.fpga_manager.sys_path + "/state", "r") as f:
            state = f.read()
        # return state without linebreak
        return state[:-1]
