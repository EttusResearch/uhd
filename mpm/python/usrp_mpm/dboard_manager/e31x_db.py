#
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
E310 dboard (RF and control) implementation module
"""

from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.periph_manager.e31x_periphs import MboardRegsControl
from usrp_mpm.mpmutils import async_exec

###############################################################################
# Main dboard control class
###############################################################################
DEFAULT_MASTER_CLOCK_RATE = 16E6
MIN_MASTER_CLK_RATE = 220E3
MAX_MASTER_CLK_RATE = 61.44E6

class E31x_db(DboardManagerBase):
    """
    Holds all dboard specific information and methods of the E31x_db dboard
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x0110]
    rx_sensor_callback_map = {
        'ad9361_temperature': 'get_catalina_temp_sensor',
        'rssi' : 'get_rssi_sensor',
        'lo_lock' : 'get_lo_lock_sensor',
    }
    tx_sensor_callback_map = {
        'ad9361_temperature': 'get_catalina_temp_sensor',
    }
    # Maps the chipselects to the corresponding devices:
    spi_chipselect = {"catalina": 0}
    ### End of overridables #################################################
    # MB regs label: Needed to access the lock bit
    mboard_regs_label = "mboard-regs"

    def __init__(self, slot_idx, **kwargs):
        super(E31x_db, self).__init__(slot_idx, **kwargs)
        self.log = get_logger("E31x_db-{}".format(slot_idx))
        self.log.trace("Initializing e31x daughterboard, slot index %d",
                       self.slot_idx)
        self.rev = int(self.device_info['rev'])
        self.log.trace("This is a rev: {}".format(chr(65 + self.rev)))
        # These will get updated during init()
        self.master_clock_rate = None
        # Predeclare some attributes to make linter happy:
        self.catalina = None
        self.eeprom_fs = None
        self.eeprom_path = None
        # Now initialize all peripherals. If that doesn't work, put this class
        # into a non-functional state (but don't crash, or we can't talk to it
        # any more):
        try:
            self._init_periphs()
            self._periphs_initialized = True
        except BaseException as ex:
            self.log.error("Failed to initialize peripherals: %s", str(ex))
            self._periphs_initialized = False

    def _init_periphs(self):
        """
        Initialize power and peripherals that don't need user-settings
        """
        self.log.debug("Loading C++ drivers...")
        # Setup Catalina / the E31x_db Manager
        self._device = lib.dboards.e31x_db_manager(
            self._spi_nodes['catalina']
        )
        self.catalina = self._device.get_radio_ctrl()
        self.log.trace("Loaded C++ drivers.")
        self._init_cat_api(self.catalina)

    def _init_cat_api(self, cat):
        """
        Propagate the C++ Catalina API into Python land.
        """
        def export_method(obj, method):
            " Export a method object, including docstring "
            meth_obj = getattr(obj, method)
            def func(*args):
                " Functor for storing docstring too "
                return meth_obj(*args)
            func.__doc__ = meth_obj.__doc__
            return func
        self.log.trace("Forwarding AD9361 methods to E31x_db class...")
        for method in [
                x for x in dir(self.catalina)
                if not x.startswith("_") and \
                        callable(getattr(self.catalina, x))]:
            self.log.trace("adding {}".format(method))
            setattr(self, method, export_method(cat, method))

    def init(self, args):
        """
        Initialize RF. Remember that we might have to do this from scratch every
        time because of the FPGA reload.
        """
        if not self._periphs_initialized:
            error_msg = "Cannot run init(), peripherals are not initialized!"
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        master_clock_rate = \
            float(args.get('master_clock_rate', DEFAULT_MASTER_CLOCK_RATE))
        assert MIN_MASTER_CLK_RATE <= master_clock_rate <= MAX_MASTER_CLK_RATE, \
            "Invalid master clock rate: {:.02f} MHz".format(
                master_clock_rate / 1e6)
        master_clock_rate_changed = master_clock_rate != self.master_clock_rate
        if master_clock_rate_changed:
            self.master_clock_rate = master_clock_rate
            self.log.debug("Updating master clock rate to {:.02f} MHz!".format(
                self.master_clock_rate / 1e6
            ))
        # Some default chains on -- needed for setup purposes
        self.catalina.set_active_chains(True, False, True, False)
        self.set_catalina_clock_rate(self.master_clock_rate)
        return True

    def tear_down(self):
        """
        De-init this object as much as possible.
        """
        self.log.trace("Tearing down E310 DB object!")
        for method in [
                x for x in dir(self.catalina)
                if not x.startswith("_") and \
                        callable(getattr(self.catalina, x))]:
            delattr(self, method)
        self.catalina = None

    def get_master_clock_rate(self):
        " Return master clock rate (== sampling rate) "
        return self.master_clock_rate

    ##########################################################################
    # Sensors
    ##########################################################################
    def get_ad9361_lo_lock(self, which):
        """
        Return LO lock status (Boolean!) of AD9361. 'which' must be
        either 'tx' or 'rx'
        """
        mboard_regs_control = \
            MboardRegsControl(self.mboard_regs_label, self.log)
        if which == "tx":
            return mboard_regs_control.get_ad9361_tx_lo_lock()
        elif which == "rx":
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
            'value': str(self.catalina.get_temperature())
        }

    def get_rssi_val(self, which):
        """
        Return the current RSSI of `which` chain in Catalina
        """
        return self.catalina.get_rssi(which)

    def get_rssi_sensor(self, chan):
        """
        Return a sensor dictionary containing the current RSSI of `which` chain
        in the RFIC
        """
        which = 'RX' + str(chan+1)
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
        self.log.trace("Setting Clock rate to {} MHz".format(rate/1e6))
        async_exec(lib.ad9361, "set_clock_rate", self.catalina, rate)
        return rate

    def catalina_tune(self, which, freq):
        """
        Async call to catalina tune
        """
        self.log.trace("Tuning {} {}".format(which, freq))
        async_exec(lib.ad9361, "tune", self.catalina, which, freq)
        return self.catalina.get_freq(which)
