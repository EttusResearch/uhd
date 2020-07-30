#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.mpmutils import to_native_str

registry = {}

def register_dboard_class(cls):
    registry[cls.__name__] = cls

class SimulatedDboardBase(DboardManagerBase):
    """
    A class to simulate daughterboards in a simulated device.
    """

    # Extra Overridables:
    # This is a list of extra methods to add to the class
    # Each element should be either a string or a tuple of
    # (string, function). Each element is added to the class
    # with the name being the string and the function being
    # the function, or the default of taking any amount of
    # arguments and returning None
    extra_methods = []

    def __init__(self, slot_idx, **kwargs):
        super().__init__(slot_idx, **kwargs)
        self.log = get_logger("sim_db-{}".format(slot_idx))
        self.device_info = {
            'pid': to_native_str(self.__class__.pids[0]),
            'serial': to_native_str("todo:serial-here"),
            'rev': to_native_str("1"),
            'eeprom_version': to_native_str('0')
        }
        self.rev = int(self.device_info['rev'])

        self.log.trace("This is a rev: {}".format(chr(65 + self.rev)))
        self._make_extra_methods()

    def init(self, args):
        self.log.trace("sim_db#init called")
        return True

    def tear_down(self):
        self.log.trace("sim_db#tear_down called")

    def _make_extra_methods(self):
        for entry in self.__class__.extra_methods:
            func = None
            prop_name = None
            if type(entry) is tuple:
                func = entry[1]
                prop_name = entry[0]
            else:
                func = lambda *args: None
                prop_name = entry
            # default values are needed because loop iterations don't create a new scope in python
            def wrapped_func(*args, prop_name=prop_name, func=func):
                self.log.debug("Called {} with args: {}".format(prop_name, args))
                return func(*args)
            setattr(self, prop_name, wrapped_func)

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

register_dboard_class(SimulatedCatalinaDboard)
