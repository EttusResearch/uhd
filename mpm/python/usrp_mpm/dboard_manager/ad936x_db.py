#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Common daughterboard code for E310 and E320
"""

from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm.mpmutils import async_exec

class AD936xDboard:
    """
    Holds all common code between E310 and E320 (AD936x-based MPM devices)
    """
    MIN_MASTER_CLK_RATE = 220e3
    MAX_MASTER_CLK_RATE = 61.44e6

    def __init__(self, mb_regs_getter):
        self.rfic = None
        self.log = None
        self.master_clock_rate = None
        self.swap_fe = False
        self._mb_regs_getter = mb_regs_getter

    def tear_down_rfic(self):
        """
        De-init this object as much as possible.
        """
        self.log.trace("Tearing down E3xx DB object!")
        for method in [
                x for x in dir(self.rfic)
                if not x.startswith("_") and callable(getattr(self.rfic, x))]:
            delattr(self, method)
        self.rfic = None

    def _init_cat_api(self, rfic):
        """
        Propagate the C++ Catalina API into Python land.
        """
        self.rfic = rfic
        def export_method(obj, method):
            " Export a method object, including docstring "
            meth_obj = getattr(obj, method)
            def func(*args):
                " Functor for storing docstring too "
                return meth_obj(*args)
            func.__doc__ = meth_obj.__doc__
            return func
        self.log.trace("Forwarding AD9361 methods to daughterboard class...")
        for method in [
                x for x in dir(self.rfic)
                if not x.startswith("_") and \
                        callable(getattr(self.rfic, x))]:
            self.log.trace("adding {}".format(method))
            setattr(self, method, export_method(rfic, method))

    def init_rfic(self, master_clock_rate):
        """
        Initialize chains and clock rate on AD936x
        """
        assert self.MIN_MASTER_CLK_RATE <= master_clock_rate <= self.MAX_MASTER_CLK_RATE, \
            "Invalid master clock rate: {:.02f} MHz".format(
                master_clock_rate / 1e6)
        master_clock_rate_changed = master_clock_rate != self.master_clock_rate
        if master_clock_rate_changed:
            self.master_clock_rate = master_clock_rate
            self.log.debug("Updating master clock rate to {:.02f} MHz!".format(
                self.master_clock_rate / 1e6
            ))
        # Some default chains on -- needed for setup purposes
        self.rfic.set_active_chains(True, False, True, False)
        self.set_catalina_clock_rate(self.master_clock_rate)

    def get_ad9361_lo_lock(self, which):
        """
        Return LO lock status (Boolean!) of AD9361. 'which' must be
        either 'tx' or 'rx'
        """
        mboard_regs_control = self._mb_regs_getter()
        if which == "tx":
            return mboard_regs_control.get_ad9361_tx_lo_lock()
        if which == "rx":
            return mboard_regs_control.get_ad9361_rx_lo_lock()
        self.log.warning("get_ad9361_lo_lock(): Invalid which param `{}'"
                         .format(which))
        return False

    def get_lo_lock_sensor(self, which):
        """
        Get sensor dict with LO lock status
        """
        self.log.trace("Reading LO Lock.")
        lo_locked = self.get_ad9361_lo_lock(which)
        return {
            'name': 'ad9361_lock',
            'type': 'BOOLEAN',
            'unit': 'locked' if lo_locked else 'unlocked',
            'value': str(lo_locked).lower(),
        }

    def get_rx_lo_lock_sensor(self, _chan):
        """
        RX-specific version of get_lo_lock_sensor() (for UHD API)
        """
        return self.get_lo_lock_sensor('rx')

    def get_tx_lo_lock_sensor(self, _chan):
        """
        TX-specific version of get_lo_lock_sensor() (for UHD API)
        """
        return self.get_lo_lock_sensor('tx')

    def get_catalina_temp_sensor(self, _):
        """
        Get temperature sensor reading of Catalina.
        """
        # Note: the unused argument is channel
        self.log.trace("Reading Catalina temperature.")
        return {
            'name': 'ad9361_temperature',
            'type': 'REALNUM',
            'unit': 'C',
            'value': str(self.rfic.get_temperature())
        }

    def get_rssi_val(self, which):
        """
        Return the current RSSI of `which` chain in Catalina
        """
        return self.rfic.get_rssi(which)

    def get_rssi_sensor(self, chan):
        """
        Return a sensor dictionary containing the current RSSI per channel
        in the RFIC
        """
        which = {
            # No frontend swap:
            False: {0: 'RX1', 1: 'RX2'},
            # Frontend swap:
            True:  {0: 'RX2', 1: 'RX1'},
        }[self.swap_fe][chan]
        return {
            'name': 'rssi',
            'type': 'REALNUM',
            'unit': 'dB',
            'value': str(self.get_rssi_val(which)),
        }

    def set_catalina_clock_rate(self, rate):
        """
        Async call to catalina set_clock_rate
        """
        self.log.trace("Setting Clock rate to {}".format(rate))
        async_exec(lib.ad9361, "set_clock_rate", self.rfic, rate)
        return rate

    def catalina_tune(self, which, freq):
        """
        Async call to catalina tune
        """
        self.log.trace("Tuning {} {}".format(which, freq))
        async_exec(lib.ad9361, "tune", self.rfic, which, freq)
        return self.rfic.get_freq(which)

    def get_master_clock_rate(self):
        " Return master clock rate (== sampling rate) "
        return self.master_clock_rate
