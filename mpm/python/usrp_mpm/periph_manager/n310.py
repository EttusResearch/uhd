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
N310 implementation module
"""

from __future__ import print_function
import os
import copy
import shutil
import subprocess
from six import iteritems, itervalues
from builtins import object
from .base import PeriphManagerBase
from ..net import get_iface_addrs
from ..net import byte_to_mac
from ..mpmtypes import SID
from usrp_mpm.rpc_server import no_rpc
from usrp_mpm import net
from usrp_mpm import dtoverlay
from usrp_mpm.xports import XportMgrUDP, XportMgrLiberio
from ..sysfs_gpio import SysFSGPIO
from .. import libpyusrp_periphs as lib

N3XX_DEFAULT_EXT_CLOCK_FREQ = 10e6
N3XX_DEFAULT_CLOCK_SOURCE = 'external'
N3XX_DEFAULT_TIME_SOURCE = 'internal'
N3XX_DEFAULT_ENABLE_GPS = True
N3XX_DEFAULT_ENABLE_FPGPIO = True

###############################################################################
# Additional peripheral controllers specific to Magnesium
###############################################################################
class TCA6424(object):
    """
    Abstraction layer for the port/gpio expander
    pins_list is  an array of different version of TCA6424 pins map.
    First element of this array corresponding to revC, second is revD etc...
    """
    pins_list = [
        (
            'PWREN-CLK-MGT156MHz',
            'NETCLK-CE',         #revC name: 'PWREN-CLK-WB-CDCM',
            'NETCLK-RESETn',     #revC name: 'WB-CDCM-RESETn',
            'NETCLK-PR0',        #revC name: 'WB-CDCM-PR0',
            'NETCLK-PR1',        #revC name: 'WB-CDCM-PR1',
            'NETCLK-OD0',        #revC name: 'WB-CDCM-OD0',
            'NETCLK-OD1',        #revC name: 'WB-CDCM-OD1',
            'NETCLK-OD2',        #revC name: 'WB-CDCM-OD2',
            'PWREN-CLK-MAINREF',
            'CLK-MAINSEL-25MHz', #revC name: 'CLK-MAINREF-SEL1',
            'CLK-MAINSEL-EX_B',  #revC name: 'CLK-MAINREF-SEL0',
            '12',
            'CLK-MAINSEL-GPS',   #revC name: '13',
            'FPGA-GPIO-EN',
            'PWREN-CLK-WB-20MHz',
            'PWREN-CLK-WB-25MHz',
            'GPS-PHASELOCK',
            'GPS-nINITSURV',
            'GPS-nRESET',
            'GPS-WARMUP',
            'GPS-SURVEY',
            'GPS-LOCKOK',
            'GPS-ALARM',
            'PWREN-GPS',
        ),
        (
            'NETCLK-PR1',
            'NETCLK-PR0',
            'NETCLK-CE',
            'NETCLK-RESETn',
            'NETCLK-OD2',
            'NETCLK-OD1',
            'NETCLK-OD0',
            'PWREN-CLK-MGT156MHz',
            'PWREN-CLK-MAINREF',
            'CLK-MAINSEL-25MHz',
            'CLK-MAINSEL-EX_B',
            '12',
            'CLK-MAINSEL-GPS',
            'FPGA-GPIO-EN',
            'PWREN-CLK-WB-20MHz',
            'PWREN-CLK-WB-25MHz',
            'GPS-PHASELOCK',
            'GPS-nINITSURV',
            'GPS-nRESET',
            'GPS-WARMUP',
            'GPS-SURVEY',
            'GPS-LOCKOK',
            'GPS-ALARM',
            'PWREN-GPS',
        )]

    def __init__(self, rev):
        # Default state: Turn on GPS power, take GPS out of reset or
        # init-survey, turn on 156.25 MHz clock
        # min Support from revC or rev = 2
        if rev == 2:
            self.pins = self.pins_list[0]
        else:
            self.pins = self.pins_list[1]

        default_val = 0x860101 if rev == 2 else 0x860780
        self._gpios = SysFSGPIO('tca6424', 0xFFF7FF, 0x86F7FF, default_val)

    def set(self, name, value=None):
        """
        Assert a pin by name
        """
        assert name in self.pins
        self._gpios.set(self.pins.index(name), value=value)

    def reset(self, name):
        """
        Deassert a pin by name
        """
        self.set(name, value=0)

    def get(self, name):
        """
        Read back a pin by name
        """
        assert name in self.pins
        return self._gpios.get(self.pins.index(name))


class FP_GPIO(object):
    """
    Abstraction layer for the front panel GPIO
    """
    EMIO_BASE = 54
    FP_GPIO_OFFSET = 32 # Bit offset within the ps_gpio_* pins

    def __init__(self, ddr):
        self._gpiosize = 12
        self._offset = self.FP_GPIO_OFFSET + self.EMIO_BASE
        self.usemask = 0xFFF
        self.ddr = ddr
        self._gpios = SysFSGPIO(
            'zynq_gpio',
            self.usemask<<self._offset,
            self.ddr<<self._offset
        )

    def set_all(self, value):
        """
        Assert all pin to the value.
        This method will convert value into binary and take the first 6 bits
        assign to 6pins.
        """
        bin_value = '{0:06b}'.format(value)
        wr_value = bin_value[-(self._gpiosize):]
        for i in range(self._gpiosize):
            if  (1<<i)&self.ddr:
                self._gpios.set(self._offset+i, wr_value[i%6])

    def set(self, index, value=None):
        """
        Assert a pin by index
        """
        assert index in range(self._gpiosize)
        self._gpios.set(self._offset+index, value)

    def reset_all(self):
        """
         Deassert all pins
        """
        for i in range(self._gpiosize):
            self._gpios.reset(self._offset+i)

    def reset(self, index):
        """
         Deassert a pin by index
        """
        assert index in range(self._gpiosize)
        self._gpios.reset(self._offset+index)

    def get_all(self):
        """
        Read back all pins
        """
        result = 0
        for i in range(self._gpiosize):
            if not (1<<i)&self.ddr:
                value = self._gpios.get(self._offset+i)
                result = (result<<1) | value
        return result

    def get(self, index):
        """
        Read back a pin by index
        """
        assert index in range(self._gpiosize)
        return self._gpios.get(self._offset+index)

###############################################################################
# Transport managers
###############################################################################
class N310XportMgrUDP(XportMgrUDP):
    eth_tables = {'eth1': 'misc-enet-regs0', 'eth2': 'misc-enet-regs1'}
    xbar_port_map = {'eth1': 0, 'eth2': 1}
    xbar_dev = "/dev/crossbar0"
    iface_config = {
        'eth1': {
            'label': 'misc-enet-regs0',
            'xbar': 0,
            'xbar_port': 0,
        },
        'eth2': {
            'label': 'misc-enet-regs1',
            'xbar': 0,
            'xbar_port': 1,
        },
    }

class N310XportMgrLiberio(XportMgrLiberio):
    max_chan = 10
    xbar_dev = "/dev/crossbar0"
    xbar_port = 2

###############################################################################
# Main Class
###############################################################################
class n310(PeriphManagerBase):
    """
    Holds N310 specific attributes and methods
    """
    #########################################################################
    # Overridables
    #
    # See PeriphManagerBase for documentation on these fields
    #########################################################################
    pids = [0x4242,]
    mboard_eeprom_addr = "e0005000.i2c"
    mboard_eeprom_max_len = 256
    mboard_info = {"type": "n3xx",
                   "product": "n310"
                  }
    mboard_max_rev = 3 # 3 == RevD
    mboard_sensor_callback_map = {
        'ref_locked': 'get_ref_lock_sensor',
        'gps_locked': 'get_gps_lock_sensor',
    }
    dboard_eeprom_addr = "e0004000.i2c"
    dboard_eeprom_max_len = 64

    # We're on a Zynq target, so the following two come from the Zynq standard
    # device tree overlay (tree/arch/arm/boot/dts/zynq-7000.dtsi)
    dboard_spimaster_addrs = ["e0006000.spi", "e0007000.spi"]
    # N310-specific settings
    # Path to N310 FPGA bin file
    # This file will always contain the current image, regardless of SFP type,
    # dboard, etc. The host is responsible for providing a compatible image
    # for the N310's current setup.
    # Override the list of updateable components
    updateable_components = {
        'fpga': {
            'callback': "update_fpga",
            'path': '/lib/firmware/n3xx.bin',
            'reset': True,
        },
        'dts': {
            'callback': "update_dts",
            'path': '/lib/firmware/n3xx.dts',
            'output': '/lib/firmware/n3xx.dtbo',
            'reset': False,
        },
    }

    @staticmethod
    def list_required_dt_overlays(eeprom_md, device_args):
        """
        Lists device tree overlays that need to be applied before this class can
        be used. List of strings.
        Are applied in order.

        eeprom_md -- Dictionary of info read out from the mboard EEPROM
        device_args -- Arbitrary dictionary of info, typically user-defined
        """
        return ['n3xx']

    def __init__(self, args):
        super(n310, self).__init__(args)
        self._device_initialized = False
        self._ext_clock_freq = None
        self._clock_source = None
        self._time_source = None
        try:
            self._init_peripherals(args)
            self._device_initialized = True
        except Exception as ex:
            self.log.error("Failed to initialize motherboard: %s", str(ex))

    def _init_peripherals(self, args):
        """
        Turn on all peripherals. This may throw an error on failure, so make
        sure to catch it.
        """
        # Init peripherals
        self.log.trace("Initializing TCA6424 port expander controls...")
        self._gpios = TCA6424(int(self.mboard_info['rev']))
        self.log.trace("Enabling power of MGT156MHZ clk")
        self._gpios.set("PWREN-CLK-MGT156MHz")
        self.enable_1G_ref_clock()
        self.enable_gps(
            enable=bool(
                args.default_args.get('enable_gps', N3XX_DEFAULT_ENABLE_GPS)
            )
        )
        self.enable_fp_gpio(
            enable=bool(
                args.default_args.get(
                    'enable_fp_gpio',
                    N3XX_DEFAULT_ENABLE_FPGPIO
                )
            )
        )
        # Init clocking
        self.enable_ref_clock(enable=True)
        self._ext_clock_freq = None
        self._clock_source = None
        self._time_source = None
        self._init_ref_clock_and_time(args.default_args)
        # Init CHDR transports
        self._xport_mgrs = {
            'udp': N310XportMgrUDP(self.log.getChild('UDP')),
            'liberio': N310XportMgrLiberio(self.log.getChild('liberio')),
        }
        # Init complete.
        self.log.info("mboard info: {}".format(self.mboard_info))

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

    def init(self, args):
        """
        Calls init() on the parent class, and then programs the Ethernet
        dispatchers accordingly.
        """
        result = super(n310, self).init(args)
        for xport_mgr in itervalues(self._xport_mgrs):
            xport_mgr.init(args)
        return result

    def deinit(self):
        """
        Clean up after a UHD session terminates.
        """
        super(n310, self).deinit()
        for xport_mgr in itervalues(self._xport_mgrs):
            xport_mgr.deinit()

    def tear_down(self):
        """
        Tear down all members that need to be specially handled before
        deconstruction.
        For N310, this means the overlay.
        """
        active_overlays = self.list_active_overlays()
        self.log.trace("N310 has active device tree overlays: {}".format(
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
        # For now, we always accept the suggestion if available, or fail
        src_address = suggested_src_address
        if src_address not in self._available_endpoints:
            raise RuntimeError("no more sids yo")
        sid = SID(src_address << 16 | dst_address)
        self.log.debug(
            "request_xport(dst=0x%04X, suggested_src_address=0x%04X, xport_type=%s): " \
            "operating on SID: %s",
            dst_address, suggested_src_address, str(xport_type), str(sid))
        # FIXME token!
        assert self.mboard_info['rpc_connection'] in ('remote', 'local')
        if self.mboard_info['rpc_connection'] == 'remote':
            return self._xport_mgrs['udp'].request_xport(
                sid,
                xport_type,
            )
        elif self.mboard_info['rpc_connection'] == 'local':
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
        assert self.mboard_info['rpc_connection'] in ('remote', 'local')
        sid = SID(xport_info['send_sid'])
        self._available_endpoints.remove(sid.src_ep)
        self.log.debug("Committing transport for SID %s, xport info: %s",
                       str(sid), str(xport_info))
        if self.mboard_info['rpc_connection'] == 'remote':
            return self._xport_mgrs['udp'].commit_xport(sid, xport_info)
        elif self.mboard_info['rpc_connection'] == 'local':
            return self._xport_mgrs['liberio'].commit_xport(sid, xport_info)

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
        self.log.trace("Setting clock source to `{}'".format(clock_source))
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
        self.log.info("Reference clock frequency is: {} MHz".format(
            ref_clk_freq/1e6
        ))
        for slot, dboard in enumerate(self.dboards):
            if hasattr(dboard, 'update_ref_clock_freq'):
                self.log.trace(
                    "Updating reference clock on dboard `{}' to {} MHz...".format(
                        slot, ref_clk_freq/1e6
                    )
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
            self.log.trace("New external reference clock frequency assignment matches "\
                           "previous assignment. Ignoring update command.")
            return
        self._ext_clock_freq = freq
        if self.get_clock_source() == 'external':
            for slot, dboard in enumerate(self.dboards):
                if hasattr(dboard, 'update_ref_clock_freq'):
                    self.log.trace(
                        "Updating reference clock on dboard `{}' to {} MHz...".format(
                            slot, freq/1e6
                        )
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
        return ['internal', 'external']

    def get_time_source(self):
        " Return the currently selected time source "
        return self._time_source

    def set_time_source(self, time_source):
        " Set a time source "
        assert time_source in self.get_time_sources()
        self.log.trace("Setting time source to `{}'".format(time_source))
        # FIXME use uio
        if time_source == 'internal':
            os.system('devmem2 0x4001000C w 0x1')
        elif time_source == 'external':
            os.system('devmem2 0x4001000C w 0x0')
        else:
            assert False

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

    def enable_1G_ref_clock(self):
        """
        Enables 125 MHz refclock for 1G interface.
        """
        self.log.trace("Enable 125 MHz Clock for 1G SFP interface.")
        self._gpios.set("NETCLK-CE")
        self._gpios.set("NETCLK-RESETn", 0)
        self._gpios.set("NETCLK-PR0", 1)
        self._gpios.set("NETCLK-PR1", 1)
        self._gpios.set("NETCLK-OD0", 1)
        self._gpios.set("NETCLK-OD1", 1)
        self._gpios.set("NETCLK-OD2", 0)
        self._gpios.set("PWREN-CLK-WB-25MHz", 1)
        self.log.trace("Finished configuring NETCLK CDCM.")
        self._gpios.set("NETCLK-RESETn", 1)

    ###########################################################################
    # Sensors
    ###########################################################################
    def get_ref_lock_sensor(self):
        """
        The N310 has no ref lock sensor, but because the ref lock is
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
        binfile_path = self.updateable_components['fpga']['path']
        if file_extension == "bit":
            self.log.trace("Converting bit to bin file and writing to {}"
                           .format(binfile_path))
            from usrp_mpm.fpga_bit_to_bin import fpga_bit_to_bin
            fpga_bit_to_bin(filepath, binfile_path, flip=True)
        elif file_extension == "bin":
            self.log.trace("Copying bin file to {}"
                           .format(binfile_path))
            shutil.copy(filepath, binfile_path)
        else:
            self.log.error("Invalid FPGA bitfile: {}"
                           .format(filepath))
            raise RuntimeError("Invalid N310 FPGA bitfile")
        # RPC server will reload the periph manager after this.
        return True

    @no_rpc
    def update_dts(self, filepath, metadata):
        """
        Update the DTS image in the filesystem
        :param filepath: path to new DTS image
        :param metadata: Dictionary of strings containing metadata
        """
        dtsfile_path = self.updateable_components['dts']['path']
        self.log.trace("Updating DTS with image at %s to %s (metadata: %s)",
                       filepath, dtsfile_path, str(metadata))
        shutil.copy(filepath, dtsfile_path)
        dtbofile_path = self.updateable_components['dts']['output']
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
                self.log.debug("`dtc' command output: \n%s", out)
        except OSError as ex:
            self.log.error("Could not execute `dtc' command. Binary probably "\
                           "not installed. Please compile DTS by hand.")
            # No fatal error here, in order not to break the current workflow
        except subprocess.CalledProcessError as ex:
            self.log.error("Error executing `dtc': %s", str(ex))
            return False
        return True

