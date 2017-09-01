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
magnesium dboard implementation module
"""

from __future__ import print_function
import struct
import time
from six import iteritems
from . import lib # Pulls in everything from C++-land
from .base import DboardManagerBase
from .. import nijesdcore
from ..uio import UIO
from ..mpmlog import get_logger
from .lmk_mg import LMK04828Mg

def create_spidev_iface(dev_node):
    """
    Create a regs iface from a spidev node
    """
    SPI_SPEED_HZ = 1000000
    SPI_MODE = 3
    SPI_ADDR_SHIFT = 8
    SPI_DATA_SHIFT = 0
    SPI_READ_FLAG = 1<<23
    SPI_WRIT_FLAG = 0
    return lib.spi.make_spidev_regs_iface(
        dev_node,
        SPI_SPEED_HZ,
        SPI_MODE,
        SPI_ADDR_SHIFT,
        SPI_DATA_SHIFT,
        SPI_READ_FLAG,
        SPI_WRIT_FLAG
    )

def create_spidev_iface_cpld(dev_node):
    """
    Create a regs iface from a spidev node
    """
    SPI_SPEED_HZ = 1000000
    SPI_MODE = 0
    SPI_ADDR_SHIFT = 16
    SPI_DATA_SHIFT = 0
    SPI_READ_FLAG = 1<<23
    SPI_WRIT_FLAG = 0
    return lib.spi.make_spidev_regs_iface(
        dev_node,
        SPI_SPEED_HZ,
        SPI_MODE,
        SPI_ADDR_SHIFT,
        SPI_DATA_SHIFT,
        SPI_READ_FLAG,
        SPI_WRIT_FLAG
    )

class Magnesium(DboardManagerBase):
    """
    Holds all dboard specific information and methods of the magnesium dboard
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x150]

    # Maps the chipselects to the corresponding devices:
    spi_chipselect = {"cpld": 0, "lmk": 1, "mykonos": 2}

    def _get_mykonos_function(self, name):
        mykfunc = getattr(self.mykonos, name)
        def func(*args):
            return mykfunc(*args)
        func.__doc__ = mykfunc.__doc__
        return func

    def __init__(self, slot_idx, **kwargs):
        super(Magnesium, self).__init__(slot_idx, **kwargs)
        self.log = get_logger("Magnesium-{}".format(slot_idx))
        self.log.trace("Initializing Magnesium daughterboard, slot index {}".format(self.slot_idx))

        self.log.debug("Loading C++ drivers for CPLD SPI.")
        self.cpld_regs = create_spidev_iface_cpld(self._spi_nodes['cpld'])

        self.log.debug("Loading C++ drivers...")
        self._device = lib.dboards.magnesium_manager(
            self._spi_nodes['mykonos'],
        )
        self.spi_lock = self._device.get_spi_lock()
        self.log.debug("Loading C++ drivers for LMK SPI.")
        self.clock_regs = create_spidev_iface(self._spi_nodes['lmk'])
        self.lmk = LMK04828Mg(self.clock_regs, self.spi_lock)
        self.mykonos = self._device.get_radio_ctrl()
        self.log.debug("Loaded C++ drivers.")

        self.log.debug("Getting Mg A uio...")
        self.radio_regs = UIO(label="dboard-regs-0", read_only=False)
        self.log.info("Radio-register UIO object successfully generated!")

        for mykfuncname in [x for x in dir(self.mykonos) if not x.startswith("_") and callable(getattr(self.mykonos, x))]:
            self.log.trace("adding {}".format(mykfuncname))
            setattr(self, mykfuncname, self._get_mykonos_function(mykfuncname))

    def init(self, args):
        """
        Execute necessary init dance to bring up dboard
        """

        def _init_clock_control(dboard_regs):
            " Create a dboard clock control object and reset it "
            dboard_clk_control = DboardClockControl(dboard_regs, self.log)
            dboard_clk_control.reset_mmcm()
            return dboard_clk_control


        self.log.info("init() called with args `{}'".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))


        self.dboard_clk_control = _init_clock_control(self.radio_regs)

        self.lmk.init()
        self.lmk.config()
        self.dboard_clk_control.enable_mmcm()
        self.log.info("Clocking Configured Successfully!")

        self.init_jesd(self.radio_regs)

        self.mykonos.start_radio()

        return True



    def cpld_peek16(self, addr):
        return self.cpld_regs.peek16(addr)

    def cpld_poke16(self, addr, data):
        return self.cpld_regs.poke16(addr, data)

    def lmk_peek(self, addr):
        return self.lmk.peek8(addr)

    def lmk_poke(self, addr, data):
        return self.lmk.poke8(addr, data)

    def lmk_setup_dbg(self):
        self.lmk.init()
        self.lmk.config()
        return "LMK Init Success"


    def init_jesd(self, uio):
        """
        Bring up the JESD link between Mykonos and the N310
        """
        self.log.trace("Creating jesdcore object")
        self.jesdcore = nijesdcore.NIMgJESDCore(uio)

        self.log.trace("Checking JESD core...")
        self.jesdcore.check_core()
        self.log.trace("Initializing LMK...")

        self.jesdcore.unreset_qpll()

        self.jesdcore.init()
        self.log.trace("Resetting Mykonos...")

        # YIKES!!! Where does this go?!? CPLD?
        # self.jesdcore.reset_mykonos() #not sure who owns the reset
        self.cpld_poke16(0x13, 0x1)
        self.cpld_poke16(0x13, 0x0)


        self.log.trace("Initializing Mykonos...")
        self.mykonos.begin_initialization()
        self.jesdcore.send_sysref_pulse()
        self.jesdcore.send_sysref_pulse()
        self.mykonos.finish_initialization()

        self.log.trace("Starting Mykonos framer...")
        self.mykonos.start_jesd_tx()
        self.jesdcore.send_sysref_pulse()
        self.log.trace("Resetting FPGA deframer...")
        self.jesdcore.init_deframer()
        self.log.trace("Resetting FPGA framer...")
        self.jesdcore.init_framer()
        self.log.trace("Starting Mykonos deframer...")
        self.mykonos.start_jesd_rx()

        self.log.trace("Enable LMFC and send")
        self.jesdcore.enable_lmfc()
        self.jesdcore.send_sysref_pulse()
        time.sleep(0.2)
        if not self.jesdcore.get_framer_status():
            raise Exception('JESD Core Framer is not synced!')
        if ((self.mykonos.get_deframer_status() & 0x7F) != 0x28):
            raise Exception('Mykonos Deframer is not synced!')
        if not self.jesdcore.get_deframer_status():
            raise Exception('JESD Core Deframer is not synced!')
        if (self.mykonos.get_framer_status() & 0xFF) != 0x3E:
            raise Exception('Mykonos Framer is not synced!')
        if (self.mykonos.get_multichip_sync_status() & 0xB) != 0xB:
            raise Exception('Mykonos multi chip sync failed!')

        self.log.trace("JESD fully synced and ready")

    def dump_jesd_core(self):
        for i in range(0x2000, 0x2110, 0x10):
            print(("0x%04X " % i), end=' ')
            for j in range(0, 0x10, 0x4):
                print(("%08X" % self.radio_regs.peek32(i + j)), end=' ')
            print("")



class DboardClockControl(object):
    """
    Control the FPGA MMCM for Radio Clock control.
    """
    # Clocking Register address constants
    RADIO_CLK_MMCM      = 0x0020
    PHASE_SHIFT_CONTROL = 0x0024
    RADIO_CLK_ENABLES   = 0x0028
    MGT_REF_CLK_STATUS  = 0x0030

    def __init__(self, regs, log):
        self.log = log
        self.regs = regs
        self.poke32 = self.regs.poke32
        self.peek32 = self.regs.peek32

    def enable_outputs(self, enable=True):
        """
        Enables or disables the MMCM outputs.
        """
        if enable:
            self.poke32(self.RADIO_CLK_ENABLES, 0x011)
        else:
            self.poke32(self.RADIO_CLK_ENABLES, 0x000)

    def reset_mmcm(self):
        """
        Uninitialize and reset the MMCM
        """
        self.log.trace("Disabling all Radio Clocks, then resetting MMCM...")
        self.enable_outputs(False)
        self.poke32(self.RADIO_CLK_MMCM, 0x1)

    def enable_mmcm(self):
        """
        Unreset MMCM and poll lock indicators

        If MMCM is not locked after unreset, an exception is thrown.
        """
        self.log.trace("Un-resetting MMCM...")
        self.poke32(self.RADIO_CLK_MMCM, 0x2)
        time.sleep(0.5) # Replace with poll and timeout TODO
        mmcm_locked = bool(self.peek32(self.RADIO_CLK_MMCM) & 0x10)
        if not mmcm_locked:
            self.log.error("MMCM not locked!")
            raise RuntimeError("MMCM not locked!")
        self.log.trace("Enabling output MMCM clocks...")
        self.enable_outputs(True)

    def check_refclk(self):
        """
        Not technically a clocking reg, but related.
        """
        return bool(self.peek32(self.MGT_REF_CLK_STATUS) & 0x1)

