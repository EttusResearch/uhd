#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
from .sim_dboard import SimulatedDboardBase

class SimulatedCatalinaDboard(SimulatedDboardBase):
    pids = [0x0110]

    extra_methods = [
        ("set_gain", lambda target, gain: gain),
        ("catalina_tune", lambda which, freq: freq),
        ("set_bw_filter", lambda which, freq: freq),
        "set_dc_offset_auto",
        "set_iq_balance_auto",
        "set_agc",
        "set_active_chains",
        "set_timing_mode",
        "data_port_loopback"
    ]

    def __init__(self, slot_idx, clock_rate_cb, **kwargs):
        super().__init__(slot_idx, **kwargs)
        self.clock_rate_cb = clock_rate_cb
        self.master_clock_rate = 122.88e6

    def get_master_clock_rate(self):
        return self.master_clock_rate

    def set_catalina_clock_rate(self, rate):
        self.clock_rate_cb(rate)
        return rate
