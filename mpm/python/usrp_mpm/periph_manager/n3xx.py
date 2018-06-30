#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
N3xx implementation module
"""

from __future__ import print_function
import os
import copy
import shutil
import subprocess
import json
import datetime
import threading
from six import iteritems, itervalues
from usrp_mpm.cores import WhiteRabbitRegsControl
from usrp_mpm.gpsd_iface import GPSDIface
from usrp_mpm.periph_manager import PeriphManagerBase
from usrp_mpm.mpmtypes import SID
from usrp_mpm.mpmutils import assert_compat_number, str2bool, poll_with_timeout
from usrp_mpm.rpc_server import no_rpc
from usrp_mpm.sys_utils import dtoverlay
from usrp_mpm.sys_utils.sysfs_thermal import read_thermal_sensor_value
from usrp_mpm.xports import XportMgrUDP, XportMgrLiberio
from usrp_mpm.periph_manager.n3xx_periphs import TCA6424
from usrp_mpm.periph_manager.n3xx_periphs import BackpanelGPIO
from usrp_mpm.periph_manager.n3xx_periphs import MboardRegsControl
from usrp_mpm.dboard_manager.magnesium import Magnesium
from usrp_mpm.dboard_manager.eiscat import EISCAT

N3XX_DEFAULT_EXT_CLOCK_FREQ = 10e6
N3XX_DEFAULT_CLOCK_SOURCE = 'internal'
N3XX_DEFAULT_TIME_SOURCE = 'internal'
N3XX_DEFAULT_ENABLE_GPS = True
N3XX_DEFAULT_ENABLE_FPGPIO = True
N3XX_DEFAULT_ENABLE_PPS_EXPORT = True
N3XX_FPGA_COMPAT = (5, 2)
N3XX_MONITOR_THREAD_INTERVAL = 1.0 # seconds

# Import daughterboard PIDs from their respective classes
MG_PID = Magnesium.pids[0]
EISCAT_PID = EISCAT.pids[0]

###############################################################################
# Transport managers
###############################################################################
class N3xxXportMgrUDP(XportMgrUDP):
    " N3xx-specific UDP configuration "
    xbar_dev = "/dev/crossbar0"
    iface_config = {
        'sfp0': {
            'label': 'misc-enet-regs0',
            'xbar': 0,
            'xbar_port': 0,
            'ctrl_src_addr': 0,
        },
        'sfp1': {
            'label': 'misc-enet-regs1',
            'xbar': 0,
            'xbar_port': 1,
            'ctrl_src_addr': 1,
        },
        'eth1': {
            'label': 'misc-enet-regs0',
            'xbar': 0,
            'xbar_port': 0,
            'ctrl_src_addr': 0,
        },
        'eth2': {
            'label': 'misc-enet-regs1',
            'xbar': 0,
            'xbar_port': 1,
            'ctrl_src_addr': 1,
        },
    }

class N3xxXportMgrLiberio(XportMgrLiberio):
    " N3xx-specific Liberio configuration "
    max_chan = 10
    xbar_dev = "/dev/crossbar0"
    xbar_port = 2

###############################################################################
# Main Class
###############################################################################
class n3xx(PeriphManagerBase):
    """
    Holds N3xx specific attributes and methods
    """
    # For every variant of the N3xx, add a line to the product map. If
    # it uses a new daughterboard, also import that PID from the dboard
    # manager class. The format of this map is:
    # (motherboard product code, (Slot-A DB PID, [Slot-B DB PID])) -> product
    product_map = {
        ('n300', (MG_PID,       )): 'n300', # Slot B is empty
        ('n310', (MG_PID, MG_PID)): 'n310',
        ('n310', (MG_PID,       )): 'n310', # If Slot B is empty, we can
                                            # still use the n310.bin image.
                                            # We'll leave this here for
                                            # debugging purposes.
        ('n310', (EISCAT_PID, EISCAT_PID)): 'eiscat',
    }

    #########################################################################
    # Overridables
    #
    # See PeriphManagerBase for documentation on these fields
    #########################################################################
    description = "N300-Series Device"
    pids = {0x4242: 'n310', 0x4240: 'n300'}
    mboard_eeprom_addr = "e0005000.i2c"
    mboard_eeprom_offset = 0
    mboard_eeprom_max_len = 256
    mboard_info = {"type": "n3xx"}
    mboard_max_rev = 5 # 5 == RevF
    mboard_sensor_callback_map = {
        'ref_locked': 'get_ref_lock_sensor',
        'gps_locked': 'get_gps_lock_sensor',
        'gps_time': 'get_gps_time_sensor',
        'gps_tpv': 'get_gps_tpv_sensor',
        'gps_sky': 'get_gps_sky_sensor',
        'temp': 'get_temp_sensor',
        'fan': 'get_fan_sensor',
    }
    dboard_eeprom_addr = "e0004000.i2c"
    dboard_eeprom_offset = 0
    dboard_eeprom_max_len = 64

    # We're on a Zynq target, so the following two come from the Zynq standard
    # device tree overlay (tree/arch/arm/boot/dts/zynq-7000.dtsi)
    dboard_spimaster_addrs = ["e0006000.spi", "e0007000.spi"]
    # N3xx-specific settings
    # Label for the mboard UIO
    mboard_regs_label = "mboard-regs"
    # Label for the white rabbit UIO
    wr_regs_label = "wr-regs"
    # Override the list of updateable components
    updateable_components = {
        'fpga': {
            'callback': "update_fpga",
            'path': '/lib/firmware/{}.bin',
            'reset': True,
        },
        'dts': {
            'callback': "update_dts",
            'path': '/lib/firmware/{}.dts',
            'output': '/lib/firmware/{}.dtbo',
            'reset': False,
        },
    }

    @classmethod
    def generate_device_info(cls, eeprom_md, mboard_info, dboard_infos):
        """
        Hard-code our product map
        """
        mb_pid = eeprom_md.get('pid')
        lookup_key = (
            n3xx.pids.get(mb_pid, 'unknown'),
            tuple([x['pid'] for x in dboard_infos]),
        )
        device_info = mboard_info
        device_info['product'] = cls.product_map.get(lookup_key, 'unknown')
        return device_info

    @staticmethod
    def list_required_dt_overlays(device_info):
        """
        Lists device tree overlays that need to be applied before this class can
        be used. List of strings.
        Are applied in order.

        eeprom_md -- Dictionary of info read out from the mboard EEPROM
        device_args -- Arbitrary dictionary of info, typically user-defined
        """
        # In the N3xx case, we name the dtbo file the same as the product.
        # N310 -> n310.dtbo, N300 -> n300.dtbo and so on.
        return [device_info['product']]

    ###########################################################################
    # Ctor and device initialization tasks
    ###########################################################################
    def __init__(self, args):
        self._tear_down = False
        self._status_monitor_thread = None
        self._ext_clock_freq = None
        self._clock_source = None
        self._time_source = None
        self._available_endpoints = list(range(256))
        self._bp_leds = None
        super(n3xx, self).__init__(args)
        if not self._device_initialized:
            # Don't try and figure out what's going on. Just give up.
            return
        try:
            self._init_peripherals(args)
        except Exception as ex:
            self.log.error("Failed to initialize motherboard: %s", str(ex))
            self._initialization_status = str(ex)
            self._device_initialized = False

    def _check_fpga_compat(self):
        " Throw an exception if the compat numbers don't match up "
        actual_compat = self.mboard_regs_control.get_compat_number()
        self.log.debug("Actual FPGA compat number: {:d}.{:d}".format(
            actual_compat[0], actual_compat[1]
        ))
        assert_compat_number(
            N3XX_FPGA_COMPAT,
            self.mboard_regs_control.get_compat_number(),
            component="FPGA",
            fail_on_old_minor=True,
            log=self.log
        )

    def _init_ref_clock_and_time(self, default_args):
        """
        Initialize clock and time sources. After this function returns, the
        reference signals going to the FPGA are valid.
        """
        self._ext_clock_freq = float(
            default_args.get('ext_clock_freq', N3XX_DEFAULT_EXT_CLOCK_FREQ)
        )
        if len(self.dboards) == 0:
            self.log.warning(
                "No dboards found, skipping setting clock and time source " \
                "configuration."
            )
            self._clock_source = N3XX_DEFAULT_CLOCK_SOURCE
            self._time_source = N3XX_DEFAULT_TIME_SOURCE
        else:
            self.set_clock_source(
                default_args.get('clock_source', N3XX_DEFAULT_CLOCK_SOURCE)
            )
            self.set_time_source(
                default_args.get('time_source', N3XX_DEFAULT_TIME_SOURCE)
            )
            self.enable_pps_out(
                default_args.get('pps_export', N3XX_DEFAULT_ENABLE_PPS_EXPORT)
            )

    def _init_meas_clock(self):
        """
        Initialize the TDC measurement clock. After this function returns, the
        FPGA TDC meas_clock is valid.
        """
        # No need to toggle reset here, simply confirm it is out of reset.
        self.mboard_regs_control.reset_meas_clk_mmcm(False)
        if not self.mboard_regs_control.get_meas_clock_mmcm_lock():
            raise RuntimeError("Measurement clock failed to init")

    def _monitor_status(self):
        """
        Status monitoring thread: This should be executed in a thread. It will
        continuously monitor status of the following peripherals:

        - GPS lock (update back-panel GPS LED)
        - REF lock (update back-panel REF LED)
        """
        self.log.trace("Launching monitor loop...")
        cond = threading.Condition()
        cond.acquire()
        while not self._tear_down:
            gps_locked = bool(self._gpios.get("GPS-LOCKOK"))
            self._bp_leds.set(self._bp_leds.LED_GPS, int(gps_locked))
            ref_locked = self.get_ref_lock_sensor()['value'] == 'true'
            self._bp_leds.set(self._bp_leds.LED_REF, int(ref_locked))
            # Now wait
            if cond.wait_for(
                    lambda: self._tear_down,
                    N3XX_MONITOR_THREAD_INTERVAL):
                break
        cond.release()
        self.log.trace("Terminating monitor loop.")

    def _init_peripherals(self, args):
        """
        Turn on all peripherals. This may throw an error on failure, so make
        sure to catch it.

        Periphals are initialized in the order of least likely to fail, to most
        likely.
        """
        # Sanity checks
        assert self.device_info.get('product') in self.product_map.values(), \
                "Device product could not be determined!"
        # Init peripherals
        self.log.trace("Initializing TCA6424 port expander controls...")
        self._gpios = TCA6424(int(self.mboard_info['rev']))
        self.log.trace("Initializing back panel LED controls...")
        self._bp_leds = BackpanelGPIO()
        self.log.trace("Enabling power of MGT156MHZ clk")
        self._gpios.set("PWREN-CLK-MGT156MHz")
        self.enable_1g_ref_clock()
        self.enable_wr_ref_clock()
        self.enable_gps(
            enable=str2bool(
                args.get('enable_gps', N3XX_DEFAULT_ENABLE_GPS)
            )
        )
        self.enable_fp_gpio(
            enable=str2bool(
                args.get(
                    'enable_fp_gpio',
                    N3XX_DEFAULT_ENABLE_FPGPIO
                )
            )
        )
        # Init Mboard Regs
        self.mboard_regs_control = MboardRegsControl(
            self.mboard_regs_label, self.log)
        self.mboard_regs_control.get_git_hash()
        self.mboard_regs_control.get_build_timestamp()
        self._check_fpga_compat()
        self._update_fpga_type()
        # Init clocking
        self.enable_ref_clock(enable=True)
        self._ext_clock_freq = None
        self._init_ref_clock_and_time(args)
        self._init_meas_clock()
        # Init CHDR transports
        self._xport_mgrs = {
            'udp': N3xxXportMgrUDP(self.log.getChild('UDP')),
            'liberio': N3xxXportMgrLiberio(self.log.getChild('liberio')),
        }
        # Spawn status monitoring thread
        self.log.trace("Spawning status monitor thread...")
        self._status_monitor_thread = threading.Thread(
            target=self._monitor_status,
            name="N3xxStatusMonitorThread",
            daemon=True,
        )
        self._status_monitor_thread.start()
        # Init complete.
        self.log.debug("Device info: {}".format(self.device_info))

    ###########################################################################
    # Session init and deinit
    ###########################################################################
    def init(self, args):
        """
        Calls init() on the parent class, and then programs the Ethernet
        dispatchers accordingly.
        """
        if not self._device_initialized:
            self.log.error(
                "Cannot run init(), device was never fully initialized!")
            return False
        # We need to disable the PPS out during clock initialization in order
        # to avoid glitches.
        enable_pps_out_state = self._default_args.get(
            'pps_export',
            N3XX_DEFAULT_ENABLE_PPS_EXPORT
        )
        self.enable_pps_out(False)
        if "clock_source" in args:
            self.set_clock_source(args.get("clock_source"))
        if "clock_source" in args or "time_source" in args:
            self.set_time_source(args.get("time_source", self.get_time_source()))
        # Uh oh, some hard coded product-related info: The N300 has no LO
        # source connectors on the front panel, so we assume that if this was
        # selected, it was an artifact from N310-related code. The user gets
        # a warning and the setting is reset to internal.
        if self.device_info.get('product') == 'n300':
            for lo_source in ('rx_lo_source', 'tx_lo_source'):
                if lo_source in args and args.get(lo_source) != 'internal':
                    self.log.warning("The N300 variant does not support "
                                     "external LOs! Setting to internal.")
                    args[lo_source] = 'internal'
        result = super(n3xx, self).init(args)
        # Now the clocks are all enabled, we can also re-enable PPS export if
        # it was turned off:
        self.enable_pps_out(enable_pps_out_state)
        for xport_mgr in itervalues(self._xport_mgrs):
            xport_mgr.init(args)
        return result

    def deinit(self):
        """
        Clean up after a UHD session terminates.
        """
        if not self._device_initialized:
            self.log.warning(
                "Cannot run deinit(), device was never fully initialized!")
            return
        super(n3xx, self).deinit()
        for xport_mgr in itervalues(self._xport_mgrs):
            xport_mgr.deinit()
        self.log.trace("Resetting SID pool...")
        self._available_endpoints = list(range(256))

    def tear_down(self):
        """
        Tear down all members that need to be specially handled before
        deconstruction.
        For N3xx, this means the overlay.
        """
        self.log.trace("Tearing down N3xx device...")
        self._tear_down = True
        if self._device_initialized:
            self._status_monitor_thread.join(3 * N3XX_MONITOR_THREAD_INTERVAL)
            if self._status_monitor_thread.is_alive():
                self.log.error("Could not terminate monitor thread! "
                               "This could result in resource leaks.")
        active_overlays = self.list_active_overlays()
        self.log.trace("N3xx has active device tree overlays: {}".format(
            active_overlays
        ))
        for overlay in active_overlays:
            dtoverlay.rm_overlay(overlay)

    ###########################################################################
    # Transport API
    ###########################################################################
    def request_xport(
            self,
            dst_address,
            suggested_src_address,
            xport_type
        ):
        """
        See PeriphManagerBase.request_xport() for docs.
        """
        # Try suggested address first, then just pick the first available one:
        src_address = suggested_src_address
        if src_address not in self._available_endpoints:
            if len(self._available_endpoints) == 0:
                raise RuntimeError(
                    "Depleted pool of SID endpoints for this device!")
            else:
                src_address = self._available_endpoints[0]
        sid = SID(src_address << 16 | dst_address)
        # Note: This SID may change its source address!
        self.log.trace(
            "request_xport(dst=0x%04X, suggested_src_address=0x%04X, xport_type=%s): " \
            "operating on temporary SID: %s",
            dst_address, suggested_src_address, str(xport_type), str(sid))
        # FIXME token!
        assert self.device_info['rpc_connection'] in ('remote', 'local')
        if self.device_info['rpc_connection'] == 'remote':
            return self._xport_mgrs['udp'].request_xport(
                sid,
                xport_type,
            )
        elif self.device_info['rpc_connection'] == 'local':
            return self._xport_mgrs['liberio'].request_xport(
                sid,
                xport_type,
            )

    def commit_xport(self, xport_info):
        """
        See PeriphManagerBase.commit_xport() for docs.

        Reminder: All connections are incoming, i.e. "send" or "TX" means
        remote device to local device, and "receive" or "RX" means this local
        device to remote device. "Remote device" can be, for example, a UHD
        session.
        """
        ## Go, go, go
        assert self.device_info['rpc_connection'] in ('remote', 'local')
        sid = SID(xport_info['send_sid'])
        self._available_endpoints.remove(sid.src_ep)
        self.log.debug("Committing transport for SID %s, xport info: %s",
                       str(sid), str(xport_info))
        if self.device_info['rpc_connection'] == 'remote':
            return self._xport_mgrs['udp'].commit_xport(sid, xport_info)
        elif self.device_info['rpc_connection'] == 'local':
            return self._xport_mgrs['liberio'].commit_xport(sid, xport_info)

    ###########################################################################
    # Device info
    ###########################################################################
    def get_device_info_dyn(self):
        """
        Append the device info with current IP addresses.
        """
        if not self._device_initialized:
            return {}
        device_info = self._xport_mgrs['udp'].get_xport_info()
        device_info.update({
            'fpga_version': "{}.{}".format(
                *self.mboard_regs_control.get_compat_number()),
            'fpga': self.updateable_components.get('fpga', {}).get('type', ""),
        })
        return device_info

    ###########################################################################
    # Clock/Time API
    ###########################################################################
    def get_clock_sources(self):
        " Lists all available clock sources. "
        self.log.trace("Listing available clock sources...")
        return ('external', 'internal', 'gpsdo')

    def get_clock_source(self):
        " Returns the currently selected clock source "
        return self._clock_source

    def set_clock_source(self, *args):
        """
        Switch reference clock.

        Throws if clock_source is not a valid value.
        """
        clock_source = args[0]
        assert clock_source in self.get_clock_sources()
        self.log.debug("Setting clock source to `{}'".format(clock_source))
        if clock_source == self.get_clock_source():
            self.log.trace("Nothing to do -- clock source already set.")
            return
        if clock_source == 'internal':
            self._gpios.set("CLK-MAINSEL-EX_B")
            self._gpios.set("CLK-MAINSEL-25MHz")
            self._gpios.reset("CLK-MAINSEL-GPS")
        elif clock_source == 'gpsdo':
            self._gpios.set("CLK-MAINSEL-EX_B")
            self._gpios.reset("CLK-MAINSEL-25MHz")
            self._gpios.set("CLK-MAINSEL-GPS")
        else: # external
            self._gpios.reset("CLK-MAINSEL-EX_B")
            self._gpios.reset("CLK-MAINSEL-GPS")
            # SKY13350 needs to be in known state
            self._gpios.set("CLK-MAINSEL-25MHz")
        self._clock_source = clock_source
        ref_clk_freq = self.get_ref_clock_freq()
        self.log.debug("Reference clock frequency is: {} MHz".format(
            ref_clk_freq/1e6
        ))
        for slot, dboard in enumerate(self.dboards):
            if hasattr(dboard, 'update_ref_clock_freq'):
                self.log.trace(
                    "Updating reference clock on dboard %d to %f MHz...",
                    slot, ref_clk_freq/1e6
                )
                dboard.update_ref_clock_freq(ref_clk_freq)

    def set_ref_clock_freq(self, freq):
        """
        Tell our USRP what the frequency of the external reference clock is.

        Will throw if it's not a valid value.
        """
        assert freq in (10e6, 20e6, 25e6)
        self.log.debug("We've been told the external reference clock " \
                       "frequency is {} MHz.".format(freq/1e6))
        if self._ext_clk_freq == freq:
            self.log.trace("New external reference clock frequency " \
                           "assignment matches previous assignment. Ignoring " \
                           "update command.")
            return
        self._ext_clock_freq = freq
        if self.get_clock_source() == 'external':
            for slot, dboard in enumerate(self.dboards):
                if hasattr(dboard, 'update_ref_clock_freq'):
                    self.log.trace(
                        "Updating reference clock on dboard %d to %f MHz...",
                        slot, freq/1e6
                    )
                    dboard.update_ref_clock_freq(freq)

    def get_ref_clock_freq(self):
        " Returns the currently active reference clock frequency"
        return {
            'internal': 25e6,
            'external': self._ext_clock_freq,
            'gpsdo': 20e6,
        }[self._clock_source]

    def get_time_sources(self):
        " Returns list of valid time sources "
        return ['internal', 'external', 'gpsdo', 'sfp0']

    def get_time_source(self):
        " Return the currently selected time source "
        return self._time_source

    def set_time_source(self, time_source):
        " Set a time source "
        assert time_source in self.get_time_sources()
        self._time_source = time_source
        self.mboard_regs_control.set_time_source(time_source, self.get_ref_clock_freq())
        if time_source == 'sfp0':
            # This error is specific to slave and master mode for White Rabbit.
            # Grand Master mode will require the external or gpsdo sources (not supported).
            if (time_source == 'sfp0' or time_source == 'sfp1') and \
              self.get_clock_source() != 'internal':
                error_msg = "Time source sfp(0|1) requires the internal clock source!"
                self.log.error(error_msg)
                raise RuntimeError(error_msg)
            if self.updateable_components['fpga']['type'] != 'WX':
                self.log.error("{} time source requires 'WX' FPGA type" \
                               .format(time_source))
                raise RuntimeError("{} time source requires 'WX' FPGA type" \
                               .format(time_source))
            # Only open UIO to the WR core once we're guaranteed it exists.
            wr_regs_control = WhiteRabbitRegsControl(
                self.wr_regs_label, self.log)
            # Wait for time source to become ready. Only applies to SFP0/1. All other
            # targets start their PPS immediately.
            self.log.debug("Waiting for {} timebase to lock..." \
                           .format(time_source))
            if not poll_with_timeout(
                    lambda: wr_regs_control.get_time_lock_status(),
                    40000, # Try for x ms... this number is set from a few benchtop tests
                    1000, # Poll every... second! why not?
                ):
                self.log.error("{} timebase failed to lock within 40 seconds. Status: 0x{:X}" \
                               .format(time_source, wr_regs_control.get_time_lock_status()))
                raise RuntimeError("Failed to lock SFP timebase.")


    def set_fp_gpio_master(self, value):
        """set driver for front panel GPIO
        Arguments:
            value {unsigned} -- value is a single bit bit mask of 12 pins GPIO
        """
        self.mboard_regs_control.set_fp_gpio_master(value)

    def get_fp_gpio_master(self):
        """get "who" is driving front panel gpio
           The return value is a bit mask of 12 pins GPIO.
           0: means the pin is driven by PL
           1: means the pin is driven by PS
        """
        return self.mboard_regs_control.get_fp_gpio_master()

    def set_fp_gpio_radio_src(self, value):
        """set driver for front panel GPIO
        Arguments:
            value {unsigned} -- value is 2-bit bit mask of 12 pins GPIO
           00: means the pin is driven by radio 0
           01: means the pin is driven by radio 1
           10: means the pin is driven by radio 2
           11: means the pin is driven by radio 3
        """
        self.mboard_regs_control.set_fp_gpio_radio_src(value)

    def get_fp_gpio_radio_src(self):
        """get which radio is driving front panel gpio
           The return value is 2-bit bit mask of 12 pins GPIO.
           00: means the pin is driven by radio 0
           01: means the pin is driven by radio 1
           10: means the pin is driven by radio 2
           11: means the pin is driven by radio 3
        """
        return self.mboard_regs_control.get_fp_gpio_radio_src()
    ###########################################################################
    # Hardware periphal controls
    ###########################################################################
    def enable_pps_out(self, enable):
        " Export a PPS/Trigger to the back panel "
        self.mboard_regs_control.enable_pps_out(enable)

    def enable_gps(self, enable):
        """
        Turn power to the GPS off or on.
        """
        self.log.trace("{} power to GPS".format(
            "Enabling" if enable else "Disabling"
        ))
        self._gpios.set("PWREN-GPS", int(bool(enable)))

    def enable_fp_gpio(self, enable):
        """
        Turn power to the front panel GPIO off or on.
        """
        self.log.trace("{} power to front-panel GPIO".format(
            "Enabling" if enable else "Disabling"
        ))
        self._gpios.set("FPGA-GPIO-EN", int(bool(enable)))

    def enable_ref_clock(self, enable):
        """
        Enables the ref clock voltage (+3.3-MAINREF). Without setting this to
        True, *no* ref clock works.
        """
        self.log.trace("{} power to reference clocks".format(
            "Enabling" if enable else "Disabling"
        ))
        self._gpios.set("PWREN-CLK-MAINREF", int(bool(enable)))

    def enable_1g_ref_clock(self):
        """
        Enables 125 MHz refclock for 1G interface.
        """
        self.log.trace("Enable 125 MHz Clock for 1G SFP interface.")
        self._gpios.set("NETCLK-CE", 1)
        self._gpios.set("NETCLK-RESETn", 0)
        self._gpios.set("NETCLK-PR0", 1)
        self._gpios.set("NETCLK-PR1", 1)
        self._gpios.set("NETCLK-OD0", 1)
        self._gpios.set("NETCLK-OD1", 1)
        self._gpios.set("NETCLK-OD2", 0)
        self._gpios.set("PWREN-CLK-WB-25MHz", 1)
        self.log.trace("Finished configuring NETCLK CDCM.")
        self._gpios.set("NETCLK-RESETn", 1)

    def enable_wr_ref_clock(self):
        """
        Enables 20 MHz WR refclk. Note that enable_1g_ref_clock() is also required for this
        interface to work, although calling it here is redundant.
        """
        self.log.trace("Enable White Rabbit reference clock.")
        self._gpios.set("PWREN-CLK-WB-20MHz", 1)

    ###########################################################################
    # Sensors
    ###########################################################################
    def get_ref_lock_sensor(self):
        """
        The N3xx has no ref lock sensor, but because the ref lock is
        historically considered a motherboard-level sensor, we will return the
        combined lock status of all daughterboards. If no dboard is connected,
        or none has a ref lock sensor, we simply return True.
        """
        self.log.trace(
            "Querying ref lock status from %d dboards.",
            len(self.dboards)
        )
        lock_status = all([
            not hasattr(db, 'get_ref_lock') or db.get_ref_lock()
            for db in self.dboards
        ])
        return {
            'name': 'ref_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    def get_temp_sensor(self):
        """
        Get temperature sensor reading of the N3xx.
        """
        self.log.trace("Reading FPGA temperature.")
        return_val = '-1'
        try:
            raw_val = read_thermal_sensor_value('fpga-thermal-zone', 'temp')
            return_val = str(raw_val/1000)
        except ValueError:
            self.log.warning("Error when converting temperature value")
        except KeyError:
            self.log.warning("Can't read temp on fpga-thermal-zone")
        return {
            'name': 'temperature',
            'type': 'REALNUM',
            'unit': 'C',
            'value': return_val
        }

    def get_fan_sensor(self):
        """
        Get cooling device reading of N3xx. In this case the speed of fan 0.
        """
        self.log.trace("Reading FPGA cooling device.")
        return_val = '-1'
        try:
            raw_val = read_thermal_sensor_value('ec-fan0', 'cur_state')
            return_val = str(raw_val)
        except ValueError:
            self.log.warning("Error when converting fan speed value")
        except KeyError:
            self.log.warning("Can't read cur_state on ec-fan0")
        return {
            'name': 'cooling fan',
            'type': 'INTEGER',
            'unit': 'rpm',
            'value': return_val
        }

    def get_gps_lock_sensor(self):
        """
        Get lock status of GPS as a sensor dict
        """
        self.log.trace("Reading status GPS lock pin from port expander")
        gps_locked = bool(self._gpios.get("GPS-LOCKOK"))
        return {
            'name': 'gps_lock',
            'type': 'BOOLEAN',
            'unit': 'locked' if gps_locked else 'unlocked',
            'value': str(gps_locked).lower(),
        }

    def get_gps_time_sensor(self):
        """
        Calculates GPS time using a TPV response from GPSd, and returns as a sensor dict

        This time is not high accuracy.
        """
        self.log.trace("Polling GPS time results from GPSD")
        with GPSDIface() as gps_iface:
            response_mode = 0
            # Read responses from GPSD until we get a non-trivial mode
            while response_mode <= 0:
                gps_info = gps_iface.get_gps_info(resp_class='tpv', timeout=15)
                self.log.trace("GPS info: {}".format(gps_info))
                response_mode = gps_info.get("mode", 0)
        time_str = gps_info.get("time", "")
        self.log.trace("GPS time string: {}".format(time_str))
        time_dt = datetime.datetime.strptime(time_str, "%Y-%m-%dT%H:%M:%S.%fZ")
        self.log.trace("GPS datetime: {}".format(time_dt))
        epoch_dt = datetime.datetime(1970, 1, 1)
        gps_time = int((time_dt - epoch_dt).total_seconds())
        return {
            'name': 'gps_time',
            'type': 'INTEGER',
            'unit': 'seconds',
            'value': str(gps_time),
        }

    def get_gps_tpv_sensor(self):
        """
        Get a TPV response from GPSd as a sensor dict
        """
        self.log.trace("Polling GPS TPV results from GPSD")
        with GPSDIface() as gps_iface:
            response_mode = 0
            # Read responses from GPSD until we get a non-trivial mode
            while response_mode <= 0:
                gps_info = gps_iface.get_gps_info(resp_class='tpv', timeout=15)
                self.log.trace("GPS info: {}".format(gps_info))
                response_mode = gps_info.get("mode", 0)
            # Return the JSON'd results
            gps_tpv = json.dumps(gps_info)
            return {
                'name': 'gps_tpv',
                'type': 'STRING',
                'unit': '',
                'value': gps_tpv,
            }

    def get_gps_sky_sensor(self):
        """
        Get a SKY response from GPSd as a sensor dict
        """
        self.log.trace("Polling GPS SKY results from GPSD")
        with GPSDIface() as gps_iface:
            # Just get the first SKY result
            gps_info = gps_iface.get_gps_info(resp_class='sky', timeout=15)
            # Return the JSON'd results
            gps_sky = json.dumps(gps_info)
            return {
                'name': 'gps_sky',
                'type': 'STRING',
                'unit': '',
                'value': gps_sky,
            }

    ###########################################################################
    # EEPROMs
    ###########################################################################
    def get_mb_eeprom(self):
        """
        Return a dictionary with EEPROM contents.

        All key/value pairs are string -> string.

        We don't actually return the EEPROM contents, instead, we return the
        mboard info again. This filters the EEPROM contents to what we think
        the user wants to know/see.
        """
        return self.mboard_info

    def set_mb_eeprom(self, eeprom_vals):
        """
        See PeriphManagerBase.set_mb_eeprom() for docs.
        """
        self.log.warn("Called set_mb_eeprom(), but not implemented!")
        raise NotImplementedError

    def get_db_eeprom(self, dboard_idx):
        """
        See PeriphManagerBase.get_db_eeprom() for docs.
        """
        try:
            dboard = self.dboards[dboard_idx]
        except KeyError:
            error_msg = "Attempted to access invalid dboard index `{}' " \
                        "in get_db_eeprom()!".format(dboard_idx)
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        db_eeprom_data = copy.copy(dboard.device_info)
        if hasattr(dboard, 'get_user_eeprom_data') and \
                callable(dboard.get_user_eeprom_data):
            for blob_id, blob in iteritems(dboard.get_user_eeprom_data()):
                if blob_id in db_eeprom_data:
                    self.log.warn("EEPROM user data contains invalid blob ID " \
                                  "%s", blob_id)
                else:
                    db_eeprom_data[blob_id] = blob
        return db_eeprom_data

    def set_db_eeprom(self, dboard_idx, eeprom_data):
        """
        Write new EEPROM contents with eeprom_map.

        Arguments:
        dboard_idx -- Slot index of dboard
        eeprom_data -- Dictionary of EEPROM data to be written. It's up to the
                       specific device implementation on how to handle it.
        """
        try:
            dboard = self.dboards[dboard_idx]
        except KeyError:
            error_msg = "Attempted to access invalid dboard index `{}' " \
                        "in set_db_eeprom()!".format(dboard_idx)
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        if not hasattr(dboard, 'set_user_eeprom_data') or \
                not callable(dboard.set_user_eeprom_data):
            error_msg = "Dboard has no set_user_eeprom_data() method!"
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        safe_db_eeprom_user_data = {}
        for blob_id, blob in iteritems(eeprom_data):
            if blob_id in dboard.device_info:
                error_msg = "Trying to overwrite read-only EEPROM " \
                            "entry `{}'!".format(blob_id)
                self.log.error(error_msg)
                raise RuntimeError(error_msg)
            if not isinstance(blob, str) and not isinstance(blob, bytes):
                error_msg = "Blob data for ID `{}' is not a " \
                            "string!".format(blob_id)
                self.log.error(error_msg)
                raise RuntimeError(error_msg)
            assert isinstance(blob, str)
            safe_db_eeprom_user_data[blob_id] = blob.encode('ascii')
        dboard.set_user_eeprom_data(safe_db_eeprom_user_data)

    ###########################################################################
    # Component updating
    ###########################################################################
    @no_rpc
    def update_fpga(self, filepath, metadata):
        """
        Update the FPGA image in the filesystem and reload the overlay
        :param filepath: path to new FPGA image
        :param metadata: Dictionary of strings containing metadata
        """
        self.log.trace("Updating FPGA with image at {} (metadata: `{}')"
                       .format(filepath, str(metadata)))
        _, file_extension = os.path.splitext(filepath)
        # Cut off the period from the file extension
        file_extension = file_extension[1:].lower()
        binfile_path = self.updateable_components['fpga']['path'].format(
            self.device_info['product'])
        if file_extension == "bit":
            self.log.trace("Converting bit to bin file and writing to {}"
                           .format(binfile_path))
            from usrp_mpm.fpga_bit_to_bin import fpga_bit_to_bin
            fpga_bit_to_bin(filepath, binfile_path, flip=True)
        elif file_extension == "bin":
            self.log.trace("Copying bin file to %s", binfile_path)
            shutil.copy(filepath, binfile_path)
        else:
            self.log.error("Invalid FPGA bitfile: %s", filepath)
            raise RuntimeError("Invalid N3xx FPGA bitfile")
        # RPC server will reload the periph manager after this.
        return True

    @no_rpc
    def _update_fpga_type(self):
        """Update the fpga type stored in the updateable components"""
        fpga_type = self.mboard_regs_control.get_fpga_type()
        self.log.debug("Updating mboard FPGA type info to {}".format(fpga_type))
        self.updateable_components['fpga']['type'] = fpga_type

    @no_rpc
    def update_dts(self, filepath, metadata):
        """
        Update the DTS image in the filesystem
        :param filepath: path to new DTS image
        :param metadata: Dictionary of strings containing metadata
        """
        dtsfile_path = self.updateable_components['dts']['path'].format(
            self.device_info['product'])
        self.log.trace("Updating DTS with image at %s to %s (metadata: %s)",
                       filepath, dtsfile_path, str(metadata))
        shutil.copy(filepath, dtsfile_path)
        dtbofile_path = self.updateable_components['dts']['output'].format(
            self.device_info['product'])
        self.log.trace("Compiling to %s...", dtbofile_path)
        dtc_command = [
            'dtc',
            '--symbols',
            '-O', 'dtb',
            '-q', # Suppress warnings
            '-o',
            dtbofile_path,
            dtsfile_path,
        ]
        self.log.trace("Executing command: `$ %s'", " ".join(dtc_command))
        try:
            out = subprocess.check_output(dtc_command)
            if out.strip() != "":
                # Keep this as debug because dtc is an external tool and
                # something could go wrong with it that's outside of our control
                self.log.debug("`dtc' command output: \n%s", out)
        except OSError as ex:
            self.log.error("Could not execute `dtc' command. Binary probably "\
                           "not installed. Please compile DTS by hand.")
            # No fatal error here, in order not to break the current workflow
        except subprocess.CalledProcessError as ex:
            self.log.error("Error executing `dtc': %s", str(ex))
            return False
        return True

    #######################################################################
    # Claimer API
    #######################################################################
    def claim(self):
        """
        This is called when the device is claimed, in case the device needs to
        run any actions on claiming (e.g., light up an LED).
        """
        if self._bp_leds is not None:
            # Light up LINK
            self._bp_leds.set(self._bp_leds.LED_LINK, 1)

    def unclaim(self):
        """
        This is called when the device is unclaimed, in case the device needs
        to run any actions on claiming (e.g., turn off an LED).
        """
        if self._bp_leds is not None:
            # Turn off LINK
            self._bp_leds.set(self._bp_leds.LED_LINK, 0)

