#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Magnesium dboard implementation module
"""

from __future__ import print_function
import os
from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.dboard_manager.mg_periphs import TCA6408, MgCPLD
from usrp_mpm.dboard_manager.mg_init import MagnesiumInitManager
from usrp_mpm.dboard_manager.mg_periphs import DboardClockControl
from usrp_mpm.cores import nijesdcore
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.uio import open_uio
from usrp_mpm.user_eeprom import BfrfsEEPROM
from usrp_mpm.mpmutils import async_exec

###############################################################################
# SPI Helpers
###############################################################################
def create_spidev_iface_lmk(dev_node):
    """
    Create a regs iface from a spidev node
    """
    return lib.spi.make_spidev_regs_iface(
        str(dev_node),
        1000000, # Speed (Hz)
        3, # SPI mode
        8, # Addr shift
        0, # Data shift
        1<<23, # Read flag
        0, # Write flag
    )

def create_spidev_iface_cpld(dev_node):
    """
    Create a regs iface from a spidev node
    """
    return lib.spi.make_spidev_regs_iface(
        str(dev_node),
        1000000, # Speed (Hz)
        0, # SPI mode
        16, # Addr shift
        0, # Data shift
        1<<23, # Read flag
        0, # Write flag
    )

def create_spidev_iface_phasedac(dev_node):
    """
    Create a regs iface from a spidev node (ADS5681)
    """
    return lib.spi.make_spidev_regs_iface(
        str(dev_node),
        1000000, # Speed (Hz)
        1, # SPI mode
        16, # Addr shift
        0, # Data shift
        0, # Read flag (phase DAC is write-only)
        0, # Write flag
    )

###############################################################################
# Main dboard control class
###############################################################################
class Magnesium(BfrfsEEPROM, DboardManagerBase):
    """
    Holds all dboard specific information and methods of the magnesium dboard
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x150]
    rx_sensor_callback_map = {
        'lowband_lo_locked': 'get_lowband_tx_lo_locked_sensor',
        'ad9371_lo_locked': 'get_ad9371_tx_lo_locked_sensor',
    }
    tx_sensor_callback_map = {
        'lowband_lo_locked': 'get_lowband_rx_lo_locked_sensor',
        'ad9371_lo_locked': 'get_ad9371_rx_lo_locked_sensor',
    }
    # Maps the chipselects to the corresponding devices:
    spi_chipselect = {"cpld": 0, "lmk": 1, "mykonos": 2, "phase_dac": 3}
    ### End of overridables #################################################
    # Class-specific, but constant settings:
    spi_factories = {
        "cpld": create_spidev_iface_cpld,
        "lmk": create_spidev_iface_lmk,
        "phase_dac": create_spidev_iface_phasedac,
    }
    #file system path to i2c-adapter/mux
    base_i2c_adapter = '/sys/class/i2c-adapter'
    # Map I2C channel to slot index
    i2c_chan_map = {0: 'i2c-9', 1: 'i2c-10'}
    # This map describes how the user data is stored in EEPROM. If a dboard rev
    # changes the way the EEPROM is used, we add a new entry. If a dboard rev
    # is not found in the map, then we go backward until we find a suitable rev
    user_eeprom = {
        2: { # RevC
            'label': "e0004000.i2c",
            'offset': 1024,
            'max_size': 32786 - 1024,
            'alignment': 1024, # FIXME check alignment is correct (block size)
        },
    }
    default_master_clock_rate = 125e6
    default_time_source = 'internal'
    default_current_jesd_rate = 2500e6

    def __init__(self, slot_idx, **kwargs):
        DboardManagerBase.__init__(self, slot_idx, **kwargs)
        self.log = get_logger("Magnesium-{}".format(slot_idx))
        self.log.trace("Initializing Magnesium daughterboard, slot index %d",
                       self.slot_idx)
        self.rev = int(self.device_info['rev'])
        self.log.trace("This is a rev: {}".format(chr(65 + self.rev)))
        # This is a default ref clock freq, it must be updated before init() is
        # called!
        self.ref_clock_freq = None
        # These will get updated during init()
        self.master_clock_rate = None
        self.current_jesd_rate = None
        # Predeclare some attributes to make linter happy:
        self.lmk = None
        self._port_expander = None
        self.mykonos = None
        self.eeprom_fs = None
        self.eeprom_path = None
        self.cpld = None
        # If _init_args is None, it means that init() hasn't yet been called.
        self._init_args = None
        # Now initialize all peripherals. If that doesn't work, put this class
        # into a non-functional state (but don't crash, or we can't talk to it
        # any more):
        try:
            self._init_periphs()
            self._periphs_initialized = True
        except Exception as ex:
            self.log.error("Failed to initialize peripherals: %s",
                           str(ex))
            self._periphs_initialized = False

    def _init_periphs(self):
        """
        Initialize power and peripherals that don't need user-settings
        """
        self._port_expander = TCA6408(self._get_i2c_dev(self.slot_idx))
        self._power_on()
        self.log.debug("Loading C++ drivers...")

        # The Mykonos TX DeFramer lane crossbar requires configuration on a per-slot
        # basis due to motherboard MGT lane swapping.
        # The RX framer lane crossbar configuration
        # is identical for both slots and is hard-coded within the Mykonos API.
        deserializer_lane_xbar = 0xD2 if self.slot_idx == 0 else 0x72

        self._device = lib.dboards.magnesium_manager(
            self._spi_nodes['mykonos'],
            deserializer_lane_xbar
        )
        self.mykonos = self._device.get_radio_ctrl()
        self.spi_lock = self._device.get_spi_lock()
        self.log.trace("Loaded C++ drivers.")
        self._init_myk_api(self.mykonos)
        self.log.debug(
            "AD9371: ARM version: {arm_ver} API version: {api_ver} "
            "Device revision: {dev_rev}".format(
                arm_ver=self.get_arm_version(),
                api_ver=self.get_api_version(),
                dev_rev=self.get_device_rev(),
            )
        )
        BfrfsEEPROM.__init__(self)
        self.log.trace("Loading SPI devices...")
        self._spi_ifaces = {
            key: self.spi_factories[key](self._spi_nodes[key])
            for key in self.spi_factories
        }
        self.cpld = MgCPLD(self._spi_ifaces['cpld'], self.log)
        self.device_info['cpld_rev'] = \
                str(self.cpld.major_rev) + '.' + str(self.cpld.minor_rev)

    def _power_on(self):
        " Turn on power to daughterboard "
        self.log.trace("Powering on slot_idx={}...".format(self.slot_idx))
        self._port_expander.set("PWR-EN-3.6V")
        self._port_expander.set("PWR-EN-1.5V")
        self._port_expander.set("PWR-EN-5.5V")
        self._port_expander.set("LED")

    def _power_off(self):
        " Turn off power to daughterboard "
        self.log.trace("Powering off slot_idx={}...".format(self.slot_idx))
        self._port_expander.reset("PWR-EN-3.6V")
        self._port_expander.reset("PWR-EN-1.5V")
        self._port_expander.reset("PWR-EN-5.5V")
        self._port_expander.reset("LED")

    def _get_i2c_dev(self, slot_idx):
        " Return the I2C path for this daughterboard "
        import pyudev
        context = pyudev.Context()
        i2c_dev_path = os.path.join(
            self.base_i2c_adapter,
            self.i2c_chan_map[slot_idx]
        )
        return pyudev.Devices.from_sys_path(context, i2c_dev_path)

    def _init_myk_api(self, myk):
        """
        Propagate the C++ Mykonos API into Python land.
        """
        def export_method(obj, method):
            " Export a method object, including docstring "
            meth_obj = getattr(obj, method)
            def func(*args):
                " Functor for storing docstring to "
                return meth_obj(*args)
            func.__doc__ = meth_obj.__doc__
            return func
        self.log.trace("Forwarding AD9371 methods to Magnesium class...")
        for method in [
                x for x in dir(self.mykonos)
                if not x.startswith("_") and \
                        callable(getattr(self.mykonos, x)) \
                        and not hasattr(self, x)]:
            self.log.trace("adding {}".format(method))
            setattr(self, method, export_method(myk, method))

    def init(self, args):
        """
        Execute necessary init dance to bring up dboard
        """
        # Sanity checks and input validation:
        self.log.debug("init() called with args `{}'".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))
        if not self._periphs_initialized:
            error_msg = "Cannot run init(), peripherals are not initialized!"
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        # Check if ref clock freq changed (would require a full init)
        ref_clk_freq_changed = False
        if 'ref_clk_freq' in args:
            new_ref_clock_freq = float(args['ref_clk_freq'])
            assert new_ref_clock_freq in (10e6, 20e6, 25e6)
            if new_ref_clock_freq != self.ref_clock_freq:
                self.ref_clock_freq = float(args['ref_clk_freq'])
                ref_clk_freq_changed = True
                self.log.debug(
                    "Updating reference clock frequency to {:.02f} MHz!"
                    .format(self.ref_clock_freq / 1e6)
                )
        assert self.ref_clock_freq is not None
        # Check if master clock freq changed (would require a full init)
        master_clock_rate = \
            float(args.get('master_clock_rate',
                           self.default_master_clock_rate))
        assert master_clock_rate in (122.88e6, 125e6, 153.6e6), \
                "Invalid master clock rate: {:.02f} MHz".format(
                    master_clock_rate / 1e6)
        master_clock_rate_changed = \
            master_clock_rate != self.master_clock_rate
        if master_clock_rate_changed:
            self.master_clock_rate = master_clock_rate
            self.log.debug(
                "Updating master clock rate to {:.02f} MHz!"
                .format(self.master_clock_rate / 1e6)
            )
        # Track if we're able to do a "fast reinit", which means there were no
        # major changes and can skip all slow initialization steps.
        fast_reinit = \
            not bool(args.get("force_reinit", False)) \
            and not master_clock_rate_changed \
            and not ref_clk_freq_changed
        if fast_reinit:
            self.log.debug(
                "Attempting fast re-init with the following settings: "
                "master_clock_rate={} MHz ref_clk_freq={}"
                .format(
                    self.master_clock_rate / 1e6,
                    self.ref_clock_freq,
                )
            )
        # Note: MagnesiumInitManager.init() can still override fast_reinit.
        # Consider it a hint.
        result = MagnesiumInitManager(self, self._spi_ifaces).init(
            args, self._init_args, fast_reinit)
        if result:
            self._init_args = args
        return result


    ##########################################################################
    # Clocking control APIs
    ##########################################################################
    def set_clk_safe_state(self):
        """
        Disable all components that could react badly to a sudden change in
        clocking. After calling this method, all clocks will be off. Calling
        _reinit() will turn them on again.

        The only downstream receiver of the clock that is not reset here are the
        lowband LOs, which are controlled through the host UHD interface.
        """
        if self._init_args is None:
            # Then we're already in a safe state
            return
        # Reset Mykonos, since it receives a copy of the clock from the LMK.
        self.cpld.reset_mykonos(keep_in_reset=True)
        with open_uio(
            label="dboard-regs-{}".format(self.slot_idx),
            read_only=False
        ) as dboard_ctrl_regs:
            # Clear the Sample Clock enables and place the MMCM in reset.
            db_clk_control = DboardClockControl(dboard_ctrl_regs, self.log)
            db_clk_control.reset_mmcm()
            # Place the JESD204b core in reset, mainly to reset QPLL/CPLLs.
            jesdcore = nijesdcore.NIJESDCore(dboard_ctrl_regs, self.slot_idx,
                                             **MagnesiumInitManager.JESD_DEFAULT_ARGS)
            jesdcore.reset()
            # The reference clock is handled elsewhere since it is a motherboard-
            # level clock.

    def set_freq(self, which, freq, wait_for_lock):
        """
        Perform asynchronous tuning. This will, under the hood, call set_freq()
        on the AD937X object, but will spin it out into an asynchronous
        execution. We do this because under some circumstances, the set_freq()
        call can take a long time to execute, and we want to release the GIL
        during that time.

        Note: This overrides the set_freq() call provided from self.mykonos.
        """
        self.log.trace("Tuning {} {} {}".format(which, freq, wait_for_lock))
        async_exec(self.mykonos, "set_freq", which, freq, wait_for_lock)
        return self.mykonos.get_freq(which)

    def _reinit(self, master_clock_rate):
        """
        This will re-run init(). We store all the settings in _init_args, so we
        will bring the device into the same state that it was before, with the
        exception of frequency and gain. Those need to be re-set by UHD in order
        not to invalidate the UHD caches.
        """
        args = self._init_args
        args["master_clock_rate"] = master_clock_rate
        args["ref_clk_freq"] = self.ref_clock_freq
        # If we add API calls to reset the cals, they need to update
        # self._init_args
        self.master_clock_rate = None # <= This will force a re-init
        self.init(args)
        # self.master_clock_rate is now OK again


    def set_master_clock_rate(self, rate):
        """
        Set the master clock rate to rate. Note this will trigger a
        re-initialization of the entire clocking, unless rate matches the
        current master clock rate.
        """
        if rate == self.master_clock_rate:
            self.log.debug(
                "New master clock rate assignment matches previous assignment. "
                "Ignoring set_master_clock_rate() command.")
            return self.master_clock_rate
        self._reinit(rate)
        return rate

    def get_master_clock_rate(self):
        " Return master clock rate (== sampling rate) "
        return self.master_clock_rate

    def update_ref_clock_freq(self, freq, **kwargs):
        """
        Call this function if the frequency of the reference clock changes
        (the 10, 20, 25 MHz one).

        If this function is called while the device is in an initialized state,
        it will also re-trigger the initialization sequence.

        No need to set the device in a safe state because (presumably) the user
        has already switched the clock rate externally. All we need to do now
        is re-initialize with the new rate.
        """
        assert freq in (10e6, 20e6, 25e6), \
                "Invalid ref clock frequency: {}".format(freq)
        self.log.trace("Changing ref clock frequency to %f MHz", freq/1e6)
        self.ref_clock_freq = freq
        if self._init_args is not None:
            self._init_args = {**self._init_args, **kwargs}
            self.log.info("Re-initializing daughter board. This may take some time.")
            self._reinit(self.master_clock_rate)
            self.log.debug("Daughter board re-initialization done.")


    ##########################################################################
    # Sensors
    ##########################################################################
    def get_ref_lock(self):
        """
        Returns True if the LMK reference is locked.

        Note: This does not return a sensor dict. The sensor API call is
        in the motherboard class.
        """
        if self.lmk is None:
            self.log.trace("LMK object not yet initialized, defaulting to " \
                           "no ref locked!")
            return False
        lmk_lock_status = self.lmk.check_plls_locked()
        self.log.trace("LMK lock status is: {}".format(lmk_lock_status))
        return lmk_lock_status

    def get_lowband_lo_lock(self, which):
        """
        Return LO lock status (Boolean!) of the lowband LOs. 'which' must be
        either 'tx' or 'rx'
        """
        assert which.lower() in ('tx', 'rx')
        return self.cpld.get_lo_lock_status(which.upper())

    def get_ad9371_lo_lock(self, which):
        """
        Return LO lock status (Boolean!) of the lowband LOs. 'which' must be
        either 'tx' or 'rx'
        """
        return self.mykonos.get_lo_locked(which.upper())

    def get_lowband_tx_lo_locked_sensor(self, chan):
        " TX lowband LO lock sensor "
        self.log.trace("Querying TX lowband LO lock status for chan %d...",
                       chan)
        lock_status = self.get_lowband_lo_lock('tx')
        return {
            'name': 'lowband_lo_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    def get_lowband_rx_lo_locked_sensor(self, chan):
        " RX lowband LO lock sensor "
        self.log.trace("Querying RX lowband LO lock status for chan %d...",
                       chan)
        lock_status = self.get_lowband_lo_lock('rx')
        return {
            'name': 'lowband_lo_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    def get_ad9371_tx_lo_locked_sensor(self, chan):
        " TX ad9371 LO lock sensor "
        self.log.trace("Querying TX AD9371 LO lock status for chan %d...", chan)
        lock_status = self.get_ad9371_lo_lock('tx')
        return {
            'name': 'ad9371_lo_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    def get_ad9371_rx_lo_locked_sensor(self, chan):
        " RX ad9371 LO lock sensor "
        self.log.trace("Querying RX AD9371 LO lock status for chan %d...", chan)
        lock_status = self.get_ad9371_lo_lock('tx')
        return {
            'name': 'ad9371_lo_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    ##########################################################################
    # Filter API
    ##########################################################################
    def set_bandwidth(self, which, bw):
        if which.lower()[0:2] in ('tx', 'dx'):
            self.log.debug("ad9371 set_tx_bandwidth bw: {}".format(bw))
            self._init_args['tx_bw'] = bw
        if which.lower()[0:2] in ('rx', 'dx'):
            self.log.debug("ad9371 set_rx_bandwidth bw: {}".format(bw))
            self._init_args['rx_bw'] = bw
        self._reinit(self.master_clock_rate)
        return bw

    def set_fir(self, name, gain, coeffs):
        self.log.debug("ad9371 set_fir name: {} gain: {} coeffs: {}".format(name, gain, coeffs))
        self.mykonos.set_fir(name, gain, coeffs)
        return

    def get_fir(self, name):
        self.log.debug("ad9371 get_fir name: {}".format(name))
        return self.mykonos.get_fir(name)

    ##########################################################################
    # Debug
    ##########################################################################
    def cpld_peek(self, addr):
        """
        Debug for accessing the CPLD via the RPC shell.
        """
        return self.cpld.peek16(addr)

    def cpld_poke(self, addr, data):
        """
        Debug for accessing the CPLD via the RPC shell.
        """
        self.cpld.poke16(addr, data)
        return self.cpld.peek16(addr)

    def dump_jesd_core(self):
        " Debug method to dump all JESD core regs "
        with open_uio(
            label="dboard-regs-{}".format(self.slot_idx),
            read_only=False
        ) as dboard_ctrl_regs:
            for i in range(0x2000, 0x2110, 0x10):
                print(("0x%04X " % i), end=' ')
                for j in range(0, 0x10, 0x4):
                    print(("%08X" % dboard_ctrl_regs.peek32(i + j)), end=' ')
                print("")

    def dbcore_peek(self, addr):
        """
        Debug for accessing the DB Core registers via the RPC shell.
        """
        with open_uio(
            label="dboard-regs-{}".format(self.slot_idx),
            read_only=False
        ) as dboard_ctrl_regs:
            rd_data = dboard_ctrl_regs.peek32(addr)
            self.log.trace("DB Core Register 0x{:04X} response: 0x{:08X}".format(addr, rd_data))
            return rd_data

    def dbcore_poke(self, addr, data):
        """
        Debug for accessing the DB Core registers via the RPC shell.
        """
        with open_uio(
            label="dboard-regs-{}".format(self.slot_idx),
            read_only=False
        ) as dboard_ctrl_regs:
            self.log.trace("Writing DB Core Register 0x{:04X} with 0x{:08X}...".format(addr, data))
            dboard_ctrl_regs.poke32(addr, data)

