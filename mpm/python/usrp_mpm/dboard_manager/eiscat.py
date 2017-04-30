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
PWR_CHAN_EN_2V5 = [ (1<<x) for x in xrange(8) ]
PWR2_5V_DC_CTRL_ENB = 1<<8
PWR2_5V_DC_PWR_EN = 1<<9
PWR2_5V_LNA_CTRL_EN = 1<<10
PWR2_5V_LMK_SPI_EN = 1<<11
PWR2_5V_ADC0_SPI_EN = 1<<12
PWR2_5V_ADC1_SPI_EN = 1<<13

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


    # def setup(self):
# 11	80	0	0	Select Master page in Analog Bank
# 53	80	0	0	Set clk divider to div-2
# 39	C0	0	0	ALWAYS WRITE 1 to this bit
# 59	20	0	0	ALWAYS WRITE 1 to this bit
# 4004	68	0	0	
# 4003	0	0	0	
# 6000	1	0	0	Reset interleaving engine for Ch A-B
# 6000	0	0	0	
# 7000	1	0	0	Reset interleaving engine for Ch C-D
# 7000	0	0	0	
# 4004	61	0	0	Select decimation filter page of JESD bank.
# 4003	41	0	0	
# 6000	E4	0	0	DDC Mode 4 for A-B and E = CH A/B N value
# 7000	E4	0	0	DDC Mode 4 for A-B and E = CH A/B N value
# 6001	4	0	0	ALWAYS WRITE 1 to this bit
# 7001	4	0	0	ALWAYS WRITE 1 to this bit
# 6002	0E	0	0	Ch A/D N value
# 7002	0E	0	0	Ch A/D N value
# 4003	0	0	0	Select analog page in JESD Bank
# 4004	6A	0	0	
# 6016	2	0	0	PLL mode 40x for A-B
# 7016	2	0	0	PLL mode 40x for C-D
# 4003	0	0	0	Select digital page in JESD Bank
# 4004	69	0	0	
# 6000	40	0	0	Enable JESD Mode control for A-B
# 6001	2	0	0	Set JESD Mode to 40x for LMFS=2441
# 7000	40	0	0	Enable JESD Mode control for C-D
# 7001	2	0	0	Set JESD Mode to 40x for LMFS=2441
# 6000	80	0	0	Set CTRL K for A-B
# 6006	0F	0	0	Set K to 16
# 7000	80	0	0	Set CTRL K for C-D
# 7006	0F	0	0	Set K to 16
# 4005	1	0	0	Disable broadcast mode
# 7001	20	0	0	SyncbAB to issue a SYNC request for all 4 channels
# 4005	0	0	0	Enable broadcast mode


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
    CORE_ID_BASE = 0x4A455344
    ADDR_BASE = 0x0000
    ADDR_OFFSET = 0x1000

# Check Core	100	4a455344	1	FFFFFFFF	check signature
	# 104		1		date should match the export number of the core. format: yymmddhh
					
# GT PLL Power Control	C	FFFC0000	0	0	Power down unused CPLLs and QPLLs
					
# GT PLL Lock Control	4	11111111	0	0	Reset CPLLs
	# 4	11111100	0	0	Unreset the ones we're using
	# wait 2 ms - alternatively, poll on the locked bit below				
	# 10	10000	0	0	Clear all CPLL sticky bits
	# 4	22	1	FF	Read CPLL locked and no unlocked stickies. Error: GT PLL failed to lock.
					
# GT RX Reset Routine	24	10	0	0	Place the RX MGTs in reset
	# 24	20	0	0	Unreset and Enable
	# 24	F0000	1	FFFF0000	Poll for reset complete for 20 ms. b1111 = number of GTs we use. Error: TX MGTs failed to reset
					
# Init FPGA JESD204B Deframer (RX)	40	2	0	0	Force assertion of ADC SYNC
	# 50	0	0	0	Data = 0 = Scrambler enabled. Data = 1 = disabled. Must match ADC settings. Set to 1 for now
	# GT RX Reset Routine (full sequence)				
	# 50	0	0	0	Stop forcing assertion of ADC SYNC
					
# Check Deframer Status (RX)	40	3000001C	1	FFFFFFFF	

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
        """
        self._gt_pll_power_control()
        self._gt_rx_reset(True)
        if not self._gt_pll_lock_control():
            raise RuntimeError("JESD CORE {} PLLs not locked!".format(self.core_idx))

    def _gt_pll_power_control(self):
        """
        Power down unused CPLLs and QPLLs
        """
        self.poke32(0x00C, 0xFFFC0000)

    def _gt_rx_reset(self, reset_only=True):
        """
        RX Reset
        """
        self.poke32(0x024, 0x10) # Place the RX MGTs in reset
        if not reset_only:
            time.sleep(.001) # Probably not necessary
            self.poke32(0x024, 0x20) # Unreset and Enable
            time.sleep(0.1) # TODO replace with poll and timeout 20 ms
            lock_status = self.peek32(0x024) & 0xFFFF0000
            if lock_status != 0xF0000:
                self.log.error(
                    "Error: TX MGTs failed to reset! Status: 0x{:x}".format(lock_status)
                )

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
        for k, v in iteritems(self.spi_chipselect):
            self._spi_nodes[k] = spi_devices[v]
        self.log.debug("spidev device node map: {}".format(self._spi_nodes))

    def init_device(self):
        """
        Execute necessary actions to bring up the daughterboard
        """
        self.log.debug("Loading C++ drivers...")
        self._device = lib.eiscat.eiscat_manager(
            self._spi_nodes['lmk'],
            self._spi_nodes['adc0'],
            self._spi_nodes['adc1'],
            # self._spi_nodes['phasedac'],
        )
        self.lmk_regs = self._device.get_clock_ctrl()
        self.adc0_regs = self._device.get_adc0_ctrl()
        self.adc1_regs = self._device.get_adc1_ctrl()
        self.spi_lock = self._device.get_spi_lock()
        self.log.debug("Loaded C++ drivers.")
        self.log.debug("Getting uio...")
        self.radio_regs = UIO(label="jesd204b-regs", read_only=False)
        jesd_id = self.radio_regs.peek32(0x100)
        self.log.trace("Reading JESD core ID: {:x}".format(jesd_id))
        if jesd_id != 0x4A455344:
            self.log.error("Cannot identify JESD core 0! Read ID: {:x}".format(jesd_id))
        jesd_id = self.radio_regs.peek32(0x1100)
        self.log.trace("Reading JESD core ID: {:x}".format(jesd_id))
        if jesd_id != 0x4A455345:
            self.log.error("Cannot identify JESD core 1! Read ID: {:x}".format(jesd_id))
            self.log.error("Cannot identify JESD core! Read ID: {:x}".format(jesd_id))
        jesd_id = self.radio_regs.peek32(0x1100)
        self.log.trace("Reading JESD core ID: {:x}".format(jesd_id))
        if jesd_id != 0x4A455345:
            self.log.error("Cannot identify JESD core! Read ID: {:x}".format(jesd_id))
        self.log.info("Radio-register UIO object successfully generated!")
        self.mmcm = MMCM(self.radio_regs, self.log)
        self.init_power(self.radio_regs)
        self.mmcm.reset()
        self.lmk = LMK04828EISCAT(self.lmk_regs, self.spi_lock, "A") # Initializes LMK
        if not self.mmcm.enable():
            self.log.error("Could not re-enable MMCM!")
            raise RuntimeError("Could not re-enable MMCM!")
        self.log.info("MMCM enabled!")
        self.adc0 = ADS54J56(self.adc0_regs, self.log)
        self.adc1 = ADS54J56(self.adc1_regs, self.log)
        self.adc0.reset()
        self.adc1.reset()
        self.log.info("ADCs resetted!")
# Send SYSREF		Pulse the "f2SendSysRefToAdcA" and "f2SendSysRefToAdcB" signals in the JESD nelist for 1 FpgaClk2x cycle. This will eventually be a timed command from the Radio.
# Initialize ADC -A		These are all SPI transactions.
# Initialize ADC -B		See the "ADC Setup" tab.
# Init FPGA JESD204B Deframer (RX) -A		See function call in "JESD204b FPGA Core Setup" tab.
# Init FPGA JESD204B Deframer (RX) -B		Same as -A.
# Send SYSREF		Same as the first Send SYSREF call.
# Check Deframer Status (RX) -A		See function call in "JESD204b FPGA Core Setup" tab.
# Check Deframer Status (RX) -B		Same as -A.

    def init_power(self, regs):
        """
        Turn on power to the dboard.

        After this function, we should never touch this register again.
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


