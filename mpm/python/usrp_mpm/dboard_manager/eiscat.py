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
EISCAT rx board implementation module
"""

import time
from six import iteritems
from ..mpmlog import get_logger
from ..uio import UIO
from . import lib
from .base import DboardManagerBase
from .lmk_eiscat import LMK04828EISCAT

N_CHANS = 8 # Chans per dboard

# Power enable pins
POWER_ENB = 0x200C # Address of the power enable register
PWR_CHAN_EN_2V5 = [(1<<x) for x in xrange(8)]
PWR2_5V_DC_CTRL_ENB = 1<<8
PWR2_5V_DC_PWR_EN = 1<<9
PWR2_5V_LNA_CTRL_EN = 1<<10
PWR2_5V_LMK_SPI_EN = 1<<11
PWR2_5V_ADC0_SPI_EN = 1<<12
PWR2_5V_ADC1_SPI_EN = 1<<13

ADC_RESET = 0x2008

def create_spidev_iface(dev_node):
    """
    Create a regs iface from a spidev node
    """
    SPI_SPEED_HZ = 1000000
    SPI_ADDR_SHIFT = 8
    SPI_DATA_SHIFT = 0
    SPI_READ_FLAG = 1<<23
    SPI_WRIT_FLAG = 0
    return lib.spi.make_spidev_regs_iface(
        dev_node,
        SPI_SPEED_HZ,
        SPI_ADDR_SHIFT,
        SPI_DATA_SHIFT,
        SPI_READ_FLAG,
        SPI_WRIT_FLAG
    )

class ADS54J56(object):
    """
    Controls for ADS54J56 ADC
    """
    def __init__(self, regs, log):
        self.log = log
        self.regs = regs

    def reset(self):
        """
        Perform reset sequence
        """
        self.log.trace("Resetting ADS54J56...")
        self.regs.poke8(0x000000, 0x81) # Analog reset
        self.regs.poke8(0x004004, 0x68) # Page = Main Digital
        self.regs.poke8(0x004003, 0x00) # Page = Main Digital
        self.regs.poke8(0x004002, 0x00) # Page = Main Digital
        self.regs.poke8(0x004001, 0x00) # Page = Main Digital
        self.regs.poke8(0x0060F7, 0x01) # Digital top reset
        self.regs.poke8(0x0070F7, 0x01) # Digital top reset
        self.regs.poke8(0x006000, 0x01) # Reset Digital (IL RESET)
        self.regs.poke8(0x007000, 0x01) # Reset Digital (IL RESET)
        self.regs.poke8(0x006000, 0x00) # Clear Reset
        self.regs.poke8(0x007000, 0x00) # Clear Reset
        self.regs.poke8(0x000011, 0x80) # Select Master page in Analog Bank
        self.regs.poke8(0x000053, 0x80) # Set clk divider to div-2
        self.regs.poke8(0x000039, 0xC0) # ALWAYS WRITE 1 to this bit
        self.regs.poke8(0x000059, 0x20) # ALWAYS WRITE 1 to this bit
        readback_test_addr = 0x11
        readback_test_val = self.regs.peek8(readback_test_addr)
        self.log.trace("ADC readback reg 0x{:x} post-init: 0x{:x}".format(
            readback_test_addr,
            readback_test_val,
        ))


    def setup(self):
        """
        Enable the ADC for streaming
        """
        self.regs.poke8(0x0011, 0x80) # Select Master page in Analog Bank
        self.regs.poke8(0x0053, 0x80) # Set clk divider to div-2
        self.regs.poke8(0x0039, 0xC0) # ALWAYS WRITE 1 to this bit
        self.regs.poke8(0x0059, 0x20) # ALWAYS WRITE 1 to this bit
        self.regs.poke8(0x4004, 0x68) #
        self.regs.poke8(0x4003, 0x00) #
        self.regs.poke8(0x6000, 0x01) # Reset interleaving engine for Ch A-B
        self.regs.poke8(0x6000, 0x00) #
        self.regs.poke8(0x7000, 0x01) # Reset interleaving engine for Ch C-D
        self.regs.poke8(0x7000, 0x00) #
        self.regs.poke8(0x4004, 0x61) # Select decimation filter page of JESD bank.
        self.regs.poke8(0x4003, 0x41) #
        self.regs.poke8(0x6000, 0xE4) # DDC Mode 4 for A-B and E = CH A/B N value
        self.regs.poke8(0x7000, 0xE4) # DDC Mode 4 for A-B and E = CH A/B N value
        self.regs.poke8(0x6001, 0x04) # ALWAYS WRITE 1 to this bit
        self.regs.poke8(0x7001, 0x04) # ALWAYS WRITE 1 to this bit
        self.regs.poke8(0x6002, 0x0E) # Ch A/D N value
        self.regs.poke8(0x7002, 0x0E) # Ch A/D N value
        self.regs.poke8(0x4003, 0x00) # Select analog page in JESD Bank
        self.regs.poke8(0x4004, 0x6A) #
        self.regs.poke8(0x6016, 0x02) # PLL mode 40x for A-B
        self.regs.poke8(0x7016, 0x02) # PLL mode 40x for C-D
        self.regs.poke8(0x4003, 0x00) # Select digital page in JESD Bank
        self.regs.poke8(0x4004, 0x69) #
        self.regs.poke8(0x6000, 0x40) # Enable JESD Mode control for A-B
        self.regs.poke8(0x6001, 0x02) # Set JESD Mode to 40x for LMFS=2441
        self.regs.poke8(0x7000, 0x40) # Enable JESD Mode control for C-D
        self.regs.poke8(0x7001, 0x02) # Set JESD Mode to 40x for LMFS=2441
        self.regs.poke8(0x6000, 0x80) # Set CTRL K for A-B
        self.regs.poke8(0x6006, 0x0F) # Set K to 16
        self.regs.poke8(0x7000, 0x80) # Set CTRL K for C-D
        self.regs.poke8(0x7006, 0x0F) # Set K to 16
        self.regs.poke8(0x4005, 0x01) # Disable broadcast mode
        self.regs.poke8(0x7001, 0x20) # SyncbAB to issue a SYNC request for all 4 channels
        self.regs.poke8(0x4005, 0x00) # Enable broadcast mode
        readback_test_addr = 0x11
        readback_test_val = self.regs.peek8(readback_test_addr)
        self.log.trace("ADC readback reg 0x{:x} post-init: 0x{:x}".format(
            readback_test_addr,
            readback_test_val,
        ))


class MMCM(object):
    """
    Controller for the MMCM inside the FPGA
    """
    RADIO_CLK_CTRL = 0x2000 # Register address

    RADIO_CLK1X_ENABLE = 1<<0
    RADIO_CLK2X_ENABLE = 1<<1
    RADIO_CLK3X_ENABLE = 1<<2
    RADIO_CLK_MMCM_RESET = 1<<3
    RADIO_CLK_VALID = 1<<4

    def __init__(self, regs, log):
        self.log = log
        self.regs = regs
        self.addr = self.RADIO_CLK_CTRL
        self.poke32 = lambda bits: self.regs.poke32(self.addr, bits)
        self.peek32 = lambda: self.regs.peek32(self.addr)

    def reset(self):
        """
        Uninitialize and reset the MMCM
        """
        self.log.trace("Resetting MMCM, disabling all clocks...")
        self.poke32(self.RADIO_CLK_MMCM_RESET)

    def enable(self):
        """
        Unreset MMCM and poll lock indicators
        """
        self.log.trace("Unresetting MMCM...")
        self.poke32(0x0000) # Take out of reset
        time.sleep(0.5) # Replace with poll and timeout TODO
        mmcm_locked = bool(self.peek32() & self.RADIO_CLK_VALID)
        if not mmcm_locked:
            self.log.error("MMCM not locked!")
            raise RuntimeError("MMCM not locked!")
        self.log.trace("Enabling output clocks on MMCM...")
        self.poke32( # Enable all the output clocks:
            self.RADIO_CLK1X_ENABLE | \
            self.RADIO_CLK2X_ENABLE | \
            self.RADIO_CLK3X_ENABLE
        )
        return True

class JesdCoreEiscat(object):
    """
    Wrapper for the JESD core. Note this core is specifically adapted for
    EISCAT, it is not general-purpose.
    """
    CORE_ID_BASE = 0x4A455344
    ADDR_BASE = 0x0000
    ADDR_OFFSET = 0x1000

    def __init__(self, regs, slot, core_idx, log):
        self.log = log
        self.regs = regs
        self.slot = slot
        assert core_idx in (0, 1)
        self.core_idx = core_idx
        self.base_addr = self.ADDR_BASE + self.ADDR_OFFSET * self.core_idx
        self.log.trace("Slot: {} JESD Core {}: Base address {}".format(
            self.slot, self.core_idx, self.base_addr
        ))
        self.peek32 = lambda addr: self.regs.peek32(self.base_addr + addr)
        self.poke32 = lambda addr, data: self.regs.poke32(self.base_addr + addr, data)
        if not self.check_core_id():
            raise RuntimeError("Could not identify JESD core!")

    def check_core_id(self):
        """
        Verify that the JESD core ID is correct.
        """
        expected_id = self.CORE_ID_BASE + self.core_idx
        core_id = self.peek32(0x100)
        self.log.trace("Reading JESD core ID: {:x}".format(core_id))
        if core_id != expected_id:
            self.log.error(
                "Cannot identify JESD core! Read ID: {:x} Expected: {:x}".format(
                    core_id, expected_id
                )
            )
            return False
        date_info = core_id = self.peek32(0x104)
        self.log.trace("Reading JESD date info: {:x}".format(date_info))
        return True

    def init(self):
        """
        Run initialization sequence on JESD core.

        Returns None, but will throw if there's a problem.
        """
        self._gt_pll_power_control()
        self._gt_rx_reset(True)
        if not self._gt_pll_lock_control():
            raise RuntimeError("JESD CORE {} PLLs not locked!".format(self.core_idx))

    def init_deframer(self):
        """
        Init FPGA JESD204B Deframer (RX)

        Returns nothing, but throws on error.
        """
        self.poke32(0x40, 0x02) # Force assertion of ADC SYNC
        self.poke32(0x50, 0x01) # Data = 0 = Scrambler enabled. Data = 1 = disabled. Must match ADC settings.
        if not self._gt_rx_reset(reset_only=False):
            raise RuntimeError("JESD Core did not come out of reset properly!")
        self.poke32(0x50, 0x00) # Stop forcing assertion of ADC SYNC

    def check_deframer_status(self):
        """
        Check deframer status (who would have thought)

        Returns True if deframer status is good.
        """
        deframer_status = self.peek32(0x40) & 0xFFFFFFFF
        if deframer_status != 0x3000001C:
            self.log.error("Unexpected JESD Core Deframer Status: {:x}".format(deframer_status))
            return False
        return True

    def _gt_pll_power_control(self):
        """
        Power down unused CPLLs and QPLLs
        """
        self.poke32(0x00C, 0xFFFC0000)

    def _gt_rx_reset(self, reset_only=True):
        """
        RX Reset. Either only puts it into reset, or also pulls it out of reset
        and makes sure lock status is correct.

        Returns True on success.
        """
        self.poke32(0x024, 0x10) # Place the RX MGTs in reset
        if not reset_only:
            time.sleep(.001) # Probably not necessary
            self.poke32(0x024, 0x20) # Unreset and Enable
            time.sleep(0.1) # TODO replace with poll and timeout 20 ms
            lock_status = self.peek32(0x024) & 0xFFFF0000
            if lock_status != 0xF0000:
                self.log.error(
                    "JESD Core {}: TX MGTs failed to reset! Status: 0x{:x}".format(self.core_idx, lock_status)
                )
                return False
        return True

    def _gt_pll_lock_control(self):
        """
        Make sure PLLs are locked
        """
        self.poke32(0x004, 0x11111111) # Reset CPLLs
        self.poke32(0x004, 0x11111100) # Unreset the ones we're using
        time.sleep(0.002) # TODO replace with poll and timeout
        self.poke32(0x010, 0x10000) # Clear all CPLL sticky bits
        lock_status = self.peek32(0x004) & 0xFF
        lock_good = bool(lock_status == 0x22)
        if not lock_good:
            self.log.error("GT PLL failed to lock! Status: 0x{:x}".format(lock_status))
        return lock_good

    def _gt_polarity_control(self):
        """
        foo
        """
        reg_val = {
            'A': {0: 0x00, 1: 0x11},
            'B': {0: 0x01, 1: 0x01},
        }
        self.log.trace(
            "JESD Core: Slot {}, ADC {}: Setting polarity control to 0b{:2b}".format(
                self.slot, self.core_idx, reg_val
            ))
        self.poke32(0x80, reg_val)



class EISCAT(DboardManagerBase):
    """
    EISCAT Daughterboard
    """
    hw_pid = 3
    special_eeprom_addrs = {"special0": "something"}
    spi_chipselect = {
        "lmk": 0,
        "adc0": 1,
        "adc1": 2
    }

    def __init__(self, spi_devices, *args, **kwargs):
        super(EISCAT, self).__init__(*args, **kwargs)
        self.log = get_logger("EISCAT")
        self.log.trace("Initializing EISCAT daughterbaord")
        if len(spi_devices) < len(self.spi_chipselect):
            self.log.error("Expected {0} spi devices, found {1} spi devices".format(
                len(self.spi_chipselect), len(spi_devices),
            ))
            raise RuntimeError("Not enough SPI devices found.")
        self._spi_nodes = {}
        self._spi_ifaces = {}
        self.log.trace("Loading SPI interfaces...")
        for k, v in iteritems(self.spi_chipselect):
            self._spi_nodes[k] = spi_devices[v]
            self._spi_ifaces[k] = create_spidev_iface(self._spi_nodes[k])
        self.log.info("Loaded SPI interfaces!")
        self.log.debug("spidev device node map: {}".format(self._spi_nodes))
        # Define some attributes so that PyLint stays quiet:
        self.radio_regs = None
        self.jesd_cores = None
        self.lmk = None
        self.adc0 = None
        self.adc1 = None

    def init_device(self):
        """
        Execute necessary actions to bring up the daughterboard

        This assumes that an appropriate overlay was loaded.
        """
        self.log.trace("Loading SPI interfaces...")
        for chip, spi_dev_node in iteritems(self._spi_nodes):
            self._spi_ifaces[chip] = create_spidev_iface(spi_dev_node)
        self.log.info("Loaded SPI interfaces!")
        self.log.debug("spidev device node map: {}".format(self._spi_nodes))
        self.log.trace("Getting uio...")
        self.radio_regs = UIO(label="jesd204b-regs", read_only=False)
        # Create JESD cores. They will also test the UIO regs on initialization.
        self.jesd_cores = [
            JesdCoreEiscat(
                self.radio_regs,
                "A", # TODO fix hard-coded slot number
                core_idx,
                self.log
            ) for core_idx in xrange(2)
        ]
        self.log.info("Radio-register UIO object successfully generated!")

        self.radio_regs.poke32(ADC_RESET, 0x0000) # TODO put this somewhere else

        # Initialize Clocking
        self.mmcm = MMCM(self.radio_regs, self.log)
        self._init_power(self.radio_regs)
        self.mmcm.reset()
        self.lmk = LMK04828EISCAT(self._spi_ifaces['lmk'], "A") # Initializes LMK
        if not self.mmcm.enable():
            self.log.error("Could not re-enable MMCM!")
            raise RuntimeError("Could not re-enable MMCM!")
        self.log.info("MMCM enabled!")
        # Initialize ADCs and JESD cores
        for i in xrange(2):
            self.jesd_cores[i].init()
        self.adc0 = ADS54J56(self._spi_ifaces['adc0'], self.log)
        self.adc1 = ADS54J56(self._spi_ifaces['adc1'], self.log)
        self.adc0.reset()
        self.adc1.reset()
        self.log.info("ADCs resetted!")
        return
        # Send SYSREF FIXME
        self.adc0.setup()
        self.adc1.setup()
        self.log.info("ADCs set up!")
        for i in xrange(2):
            self.jesd_cores[i].init_deframer()
        # Send SYSREF FIXME
        for i in xrange(2):
            if not self.jesd_cores[i].check_deframer_status():
                raise RuntimeError("JESD Core {}: Deframer status not lookin' so good!".format(i))
        ## END OF THE JEPSON SEQUENCE ##


    def _init_power(self, regs):
        """
        Turn on power to the dboard.

        After this function, we should never touch this register again (other
        than turning it off again).
        """
        reg_val = PWR2_5V_DC_CTRL_ENB
        self.log.trace("Asserting power ctrl enable ({})...".format(bin(reg_val)))
        regs.poke32(POWER_ENB, reg_val)
        time.sleep(0.001)
        reg_val = reg_val \
                | PWR2_5V_DC_CTRL_ENB \
                | PWR2_5V_DC_PWR_EN \
                | PWR2_5V_LNA_CTRL_EN \
                | PWR2_5V_LMK_SPI_EN | PWR2_5V_ADC0_SPI_EN #| PWR2_5V_ADC1_SPI_EN
        regs.poke32(POWER_ENB, reg_val)
        self.log.trace("Asserting power enable for all the chips ({})...".format(bin(reg_val)))
        time.sleep(0.1)
        for chan in xrange(8):
            reg_val = reg_val | PWR_CHAN_EN_2V5[chan]
        self.log.trace("Asserting power enable for all the channels ({})...".format(bin(reg_val)))
        regs.poke32(POWER_ENB, reg_val)

    def _deinit_power(self, regs):
        """
        Turn off power to the dboard.
        """
        self.log.trace("Disabling power to the daughterboard...")
        regs.poke32(POWER_ENB, 0x0000)

