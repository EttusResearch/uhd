#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
DAC37J82 driver for use with Rhodium
"""

import time
from builtins import object
from ..mpmlog import get_logger

class DAC37J82Rh(object):
    """
    This class provides an interface to configure the DAC37J82 IC through SPI.
    """

    DAC_VENDOR_ID = 0b01
    DAC_VERSION_ID = 0b010 # Version used in Rhodium Rev. A

    def __init__(self, slot_idx, regs_iface, parent_log=None):
        self.log = parent_log.getChild("DAC37J82") if parent_log is not None \
            else get_logger("DAC37J82-{}".format(slot_idx))
        self.slot_idx = slot_idx
        self.regs = regs_iface
        assert hasattr(self.regs, 'peek16')
        assert hasattr(self.regs, 'poke16')

        def _verify_chip_id():
            chip_id = self.regs.peek16(0x7F) & 0x001F
            self.log.trace("DAC Vendor & Version ID: 0x{:X}".format(chip_id))
            if chip_id != ((self.DAC_VENDOR_ID << 3) | self.DAC_VERSION_ID):
                self.log.error("Wrong Vendor & Version 0x{:X}".format(chip_id))
                return False
            return True

        self.reset()

        if not _verify_chip_id():
            raise RuntimeError("Unable to locate DAC37J82")

        # Define variable configuration per slot.
        # The JESD lanes going to the DAC pins are swapped differently:
        #   DBA: 0 -> 0  /  1 -> 1  /  2 -> 2  /  3 -> 3
        #   DBB: 0 -> 0  /  1 -> 1  /  2 -> 3  /  3 -> 2
        # Therefore, depending on the DB that is being configured, we need
        # to change the JESD lanes internal routing in the DAC to compensate
        # for the board traces swapping.
        self.lanes_ids_1   = {0: 0x0044, 1: 0x0046}[self.slot_idx] # config70
        self.lanes_ids_2   = {0: 0x190A, 1: 0x110A}[self.slot_idx] # config71
        self.octetpath_sel = {0: 0x0123, 1: 0x0132}[self.slot_idx] # config95

        self.init()


    def tx_enable(self, enable=False):
        """
        Enable/disable the analog TX output.
        """
        enable_bit = 0b1 if enable else 0b0
        prev_val = self.regs.peek16(0x03)
        self.regs.poke16(0x03, prev_val | enable_bit)


    def pokes16(self, addr_vals):
        """
        Apply a series of pokes.
        pokes16((0,1),(0,2)) is the same as calling poke16(0,1), poke16(0,2).
        """
        for addr, val in addr_vals:
            self.regs.poke16(addr, val)


    def init(self):
        """
        Basic init that disables the analog output.
        """
        self.tx_enable(False) # Set TXENABLE low at the DAC
        self.log.trace("DAC's Analog TX output is disabled.")

    def reset(self):
        """
        Reset the DAC state
        """
        self.regs.poke16(0x02, 0x2002) # Deassert the reset for the SIF registers
        self.regs.poke16(0x02, 0x2003) # Assert the reset for the SIF registers

    def config(self):
        """
        Check the clock status, and write configuration values!
        """
        def _check_pll_lock():
            pll_ool_alarms = self.regs.peek16(0x6C)
            if (pll_ool_alarms & 0x0008) != 0x0000:
                self.log.warning("PLL reporting unlocked... Status: 0x{:x}"
                                 .format(pll_ool_alarms))
                return False
            return True

        self.log.trace("Reset DAC & Clear alarm bits")
        self.reset()
        self.regs.poke16(0x6C, 0x0000) # Clear alarm bits for PLLs

        self.log.trace("DAC Configuration.")
        self.pokes16((
            (0x00, 0x001B), # config0: Interpolation 1x; ALARM enabled w/ pos. logic.
            (0x01, 0x0003), # config1: Rewriting reserved default values.
            (0x02, 0x0002), # config2: Data not zero when link not established; 2's comp. arrives at input.
            (0x03, 0x9300), # config3: Coarse DAC = 10 (9+1); TXENABLE internal is kept low.
            (0x04, 0x0000), # config4: Not masking any lane errors or FIFO flags.
            (0x05, 0x0000), # config5: Not masking any SYSREF errors, PAPs, or PLL locks.
            (0x06, 0x0000), # config6: Not masking any lane short test or loss of signal.
            (0x08, 0x0000), # config8: DAC A offset correction set to zero (default).
            (0x09, 0x0000), # config9: DAC B offset correction set to zero (default).
            (0x0A, 0x0000), # config10: DAC C offset correction set to zero (default).
            (0x0B, 0x0000), # config11: DAC D offset correction set to zero (default).
            (0x0C, 0x0400), # config12: Default quadrature correction gain A (AB path).
            (0x0D, 0x0400), # config13: Default coarse mixing options; default quadrature correction gain B (AB path).
            (0x0E, 0x0400), # config14: Default quadrature correction gain A (CD path).
            (0x0F, 0x0400), # config15: No output delays to the DACs; default quadrature correction gain A (CD path).
            (0x10, 0x0000), # config16: Default QMC correction phase (AB path).
            (0x11, 0x0000), # config17: Default QMC correction phase (AD path).
            (0x12, 0x0000), # config18: Phase offset for NCO in DACAB path (default).
            (0x13, 0x0000), # config19: Phase offset for NCO in DACAB path (default).
            (0x14, 0x0000), # config20: Lower 16 bits of NCO Frequency adjust word for DACAB path (default).
            (0x15, 0x0000), # config21: Middle 16 bits of NCO Frequency adjust word for DACAB path (default).
            (0x16, 0x0000), # config22: Upper 16 bits of NCO Frequency adjust word for DACAB path (default).
            (0x17, 0x0000), # config23: Lower 16 bits of NCO Frequency adjust word for DACCD path (default).
            (0x18, 0x0000), # config24: Middle 16 bits of NCO Frequency adjust word for DACCD path (default).
            (0x19, 0x0000), # config25: Upper 16 bits of NCO Frequency adjust word for DACCD path (default).
            (0x1A, 0x0023), # config26: DAC PLL in sleep mode; DAC C & D in sleep mode.
            (0x1B, 0x0000), # config27: Testing settings (default).
            (0x1E, 0x2222), # config30: Sync source for the QMC offset and correction: SYSREF only (Based on VST2).
            (0x1F, 0x2220), # config31: Sync source for mixers and NCO accums.: SYSREF only (Based on VST2).
            (0x20, 0x0000), # config32: Sync source for the dithering, PA protection, and FIR filter blocks: none (Based on VST2).
            (0x22, 0x1B1B), # config34: JESD and DAC routing paths (default).
            (0x23, 0x01FF), # config35: SLEEP signal from pin allowed to reach all blocks, the pin is not even used.
            (0x24, 0x0000), # config36: SYSREF syncs clock dividers: use no pulses yet.
            (0x25, 0x2000), # config37: DACCLK divider to generate JESD clock: div2. (TI recommendation).
            (0x26, 0x0000), # config38: Dithering disabled default).
            (0x2D, 0x0000), # config45: Power amplifier protection settings (default).
            (0x2E, 0xFFFF), # config46: Power amplifier protection threshold (default).
            (0x2F, 0x0004), # config47: Default values.
            (0x30, 0x0000), # config48: Constant value sent to DAC when sifdac_ena is asserted (default).
            (0x31, 0x1000), # config49: DAC PLL in reset and disabled. DACCLK is 491.52 MHz since we bypass the PLL.
            (0x32, 0x0000), # config50: DAC PLL's VCO feedback and prescaler divided (not used).
            (0x33, 0x0000), # config51: DAC PLL VCO and CP settings (not used).
            (0x34, 0x0000), # config52: SYNCB electrical configuration (default @ 1.2V CMV).
            (0x3B, 0x0000), # config59: SerDes PLL's reference. Source: DACCLK / Divider: 1 (0 +1).
            (0x3C, 0x1828), # config60: SerDes PLL Control: high loop BW; high range VCO; multiply factor x5.
            (0x3D, 0x0088), # config61: Upper configuration info for SerDes receivers (TI recommendation).
            (0x3E, 0x0128), # config62: Upper configuration  for SerDes receivers:AC coupling; half rate; 20-bit width.
            (0x3F, 0x0000), # config63: No SerDes lanes inversion (default).
            (0x46, self.lanes_ids_1), # config70: JESD ID for lanes 0, 1, and 2.
            (0x47, self.lanes_ids_2), # config71: JESD ID for lanes 3, 4, and 5.
            (0x48, 0x31C3), # config72: JESD ID for lanes 6, and 7; JESD204B supported version and class (default).
            (0x49, 0x0000), # config73: JESD lanes assignment to links (default 0).
            (0x4A, 0x0F3E), # config74: Lanes 0-3 enabled; test seq. disabled; disable clocks to C/D paths.
            (0x4B, 0x1700), # config75: RBD = 24 (23 + 1) (Release Buffer Delay); F = 1 (octets per frame).
            (0x4C, 0x1703), # config76: K = 24 (23 + 1) (frames in multiframe); L = 4 (3 + 1) (number of lanes).
            (0x4D, 0x0100), # config77: M = 2 (1+1) (number of converters); S = 1 (0+1) (number of samples per frame).
            (0x4E, 0x0F4F), # config78: HD = 1 (High Density mode enabled, samples split across lanes).
            (0x4F, 0x1CC1), # config79: Match /R/ char; ILA is supported at TX.
            (0x50, 0x0000), # config80: Lane config data (link0), not used by 37J82 (default).
            (0x51, 0x00FF), # config81: Erros that cause a SYNC request (link0): all selected (default).
            (0x52, 0x00FF), # config82: Errors that are counted in err_c (link0): all selected (default).
            (0x53, 0x0000), # config83: Lane config data (link1), not used by 37J82 (default).
            (0x54, 0x0000), # config84: Erros that cause a SYNC request (link1): none selected.
            (0x55, 0x0000), # config85: Errors that are counted in err_c (link1): none selected.
            (0x56, 0x0000), # config86: Lane config data (link2), not used by 37J82 (default).
            (0x57, 0x0000), # config87: Erros that cause a SYNC request (link2): none selected.
            (0x58, 0x0000), # config88: Errors that are counted in err_c (link2): none selected.
            (0x59, 0x0000), # config89: Lane config data (link3), not used by 37J82 (default).
            (0x5A, 0x0000), # config90: Erros that cause a SYNC request (link2): none selected.
            (0x5B, 0x0000), # config91: Errors that are counted in err_c (link3): none selected.
            (0x5C, 0x0000), # config92: Links 3:1 don't use SYSREF pulses; link 0 uses no pulses yet.
            (0x5E, 0x0000), # config94: Cheksum bits for ILA, not used in 37J82 (default).
            (0x5F, self.octetpath_sel), # config95: Mapping SerDes lanes (0-3) to JESD lanes.
            (0x60, 0x4567), # config96: Mapping SerDes lanes (4-7) to JESD lanes (default).
            (0x61, 0x0001), # config97: Use only link 0 to trigger the SYNCB LVDS output.
            (0x64, 0x0703), # config100: Write to lane 0 errors to clear them (based on VST2).
            (0x65, 0x0703), # config101: Write to lane 1 errors to clear them (based on VST2).
            (0x66, 0x0703), # config102: Write to lane 2 errors to clear them (based on VST2).
            (0x67, 0x0703), # config103: Write to lane 3 errors to clear them (based on VST2).
            (0x68, 0x0703), # config104: Write to lane 4 errors to clear them (based on VST2).
            (0x69, 0x0703), # config105: Write to lane 5 errors to clear them (based on VST2).
            (0x6A, 0x0703), # config106: Write to lane 6 errors to clear them (based on VST2).
            (0x6B, 0x0703), # config107: Write to lane 7 errors to clear them (based on VST2).
            (0x6C, 0x0000), # config108: Rewrite the PLLs alarm bits clearing register.
            (0x6D, 0x0000), # config109: JESD short test alarms (default).
            (0x6E, 0x0000), # config110: Delay fractional filter settings (default).
            (0x6F, 0x0000), # config111: Delay fractional filter settings (default).
            (0x70, 0x0000), # config112: Delay fractional filter settings (default).
            (0x71, 0x0000), # config113: Delay fractional filter settings (default).
            (0x72, 0x0000), # config114: Delay fractional filter settings (default).
            (0x73, 0x0000), # config115: Delay fractional filter settings (default).
            (0x74, 0x0000), # config116: Delay fractional filter settings (default).
            (0x75, 0x0000), # config117: Delay fractional filter settings (default).
            (0x76, 0x0000), # config118: Delay fractional filter settings (default).
            (0x77, 0x0000), # config119: Delay fractional filter settings (default).
            (0x78, 0x0000), # config120: Delay fractional filter settings (default).
            (0x79, 0x0000), # config121: Delay fractional filter settings (default).
            (0x7A, 0x0000), # config122: Delay fractional filter settings (default).
            (0x7B, 0x0000), # config123: Delay fractional filter settings (default).
            (0x7C, 0x0000), # config124: Delay fractional filter settings (default).
            (0x7D, 0x0000), # config125: Delay fractional filter settings (default).
            (0x02, 0x2002), # Deassert the reset for the SIF registers
        ))
        self.log.trace("DAC register dump finished.")

        self.log.trace("Polling for PLL lock...")
        locked = False
        for _ in range(6):
            time.sleep(0.001)
            # Clear stickies possibly?
            self.regs.poke16(0x6C, 0x0000) # Clear alarm bits for PLLs
            if _check_pll_lock():
                locked = True
                self.log.info("DAC PLL Locked!")
                break
        if not locked:
            raise RuntimeError("DAC PLL did not lock! Check the logs for details.")


    def enable_sysref_capture(self, enabled=False):
        """
        Enable the SYSREF capture block, and enable divider's reset.
        """
        self.log.trace("%s DAC SYSREF capture...",
                       {True: 'Enabling', False: 'Disabling'}[enabled])
        cdrvser_sysref_mode = 0b001 if enabled else 0b000
        sysref_mode_link0 = 0b001 if enabled else 0b000
        self.regs.poke16(0x24, cdrvser_sysref_mode << 4) # Enable next SYSREF to reset the clock dividers.
        self.regs.poke16(0x5C, sysref_mode_link0 << 0)   # Enable next SYSREF pulse capture for link 0.
        self.log.trace("DAC SYSREF capture %s." % {True: 'enabled', False: 'disabled'}[enabled])


    def init_deframer(self):
        """
        Initialize the DAC's framer.
        """
        self.log.trace("Initializing framer...")
        self.pokes16((
            (0x4A, 0x0F3F), # config74: Deassert JESD204B block reset.
            (0x4A, 0x0F21), # config74: Set JESD204B to exit init state.
        ))
        self.log.trace("DAC deframer initialized.")


    def check_deframer_status(self):
        """
        This function checks the status of the framer by checking alarms.
        """
        ALARM_ERRORS_DESCRIPTION = {
            15 : "Multiframe alignment error",
            14 : "Frame alignment error",
            13 : "Link configuration error",
            12 : "Elastic buffer overflow",
            11 : "Elastic buffer match error",
            10 : "Code synchronization error",
            9  : "8b/10b not-in-table code error",
            8  : "8b/10b disparity error",
            3  : "FIFO write_error",
            2  : "FIFO write_full",
            1  : "FIFO read_error",
            0  : "FIFO read_empty"
        }

        self.log.trace("Checking DAC's deframer status.")
        # Clear lane alarms.
        for addr in (0x64, 0x65, 0x66, 0x67):
            self.regs.poke16(addr, 0x0000)
        time.sleep(0.001)

        # Read lane's alarms
        lanes_alarms_ary = []
        lane = 0
        for addr in (0x64, 0x65, 0x66, 0x67):
            lanes_alarms_ary.insert(lane, self.regs.peek16(addr) & 0xFF0F)
            self.log.trace("Lane {} alarms rb: 0x{:X}".format(lane, lanes_alarms_ary[lane]))
            lane += 1

        enable_analog_output = True
        # Report warnings based on an error matrix (register_width * lanes).
        errors_ary = []
        for error in range(0, 16):
            errors_ary.insert(error, [])
            # Extract errors from lanes.
            for lane in range(0, len(lanes_alarms_ary)):
                if lanes_alarms_ary[lane] & (0b1 << error) > 0:
                    errors_ary[error].append(lane)
            if len(errors_ary[error]) > 0:
                enable_analog_output = False
                self.log.warning(ALARM_ERRORS_DESCRIPTION[error] +
                                 " in lane(s): " + ' '.join(map(str, errors_ary[error])))

        self.tx_enable(enable_analog_output)
        self.log.debug("%s analog TX output.",
                       {True: 'Enabling', False: 'Disabling'}[enable_analog_output])
        return enable_analog_output


    def test_mode(self, mode='PRBS-31', lane=0):
        """
        This method enables the DAC's test mode to verify the SerDes.
        Users should monitor the ALARM pin to see the results of the test.
        If the test is failing, ALARM will be high (or toggling if marginal).
        If the test is passing, the ALARM will be low.
        """
        MODE_VAL = {'OFF': 0x0, 'PRBS-7': 0x2, 'PRBS-23': 0x3, 'PRBS-31': 0x4}
        assert mode.upper() in MODE_VAL
        assert lane in range(0, 8)
        self.log.debug("Setting test mode for lane {} at the DAC: {}.".format(lane, mode))
        # To run the PRBS test on the DAC, users first need to setup the DAC for
        # normal use, then make the following SPI writes:
        # 1. config74, set bits 4:0 to 0x1E to disable JESD clock.
        addr = 0x4A
        rb = self.regs.peek16(addr)
        data_w = (rb & ~0x001F) | 0x001E if mode != 'OFF' else 0x0F3E
        self.log.trace("Writing register {:02X} with {:04X}".format(addr, data_w))
        self.regs.poke16(addr, data_w)
        # 2. config61, set bits 14:12 to 0x2 to enable the 7-bit PRBS test pattern; or
        #              set bits 14:12 to 0x3 to enable the 23-bit PRBS test pattern; or
        #              set bits 14:12 to 0x4 to enable the 31-bit PRBS test pattern.
        addr = 0x3D
        rb = self.regs.peek16(addr)
        data_w = (rb & ~0x7000) | (MODE_VAL[mode] << 12)
        self.log.trace("Writing register {:02X} with {:04X}".format(addr, data_w))
        self.regs.poke16(addr, data_w)
        # 3. config27, set bits 11:8 to 0x3 to output PRBS testfail on ALARM pin.
        # 4. config27, set bits 14:12 to the lane to be tested (0 through 7).
        addr = 0x1B
        rb = self.regs.peek16(addr)
        data_w = (rb & ~0x7F00) | (0x3 << 8) | (lane << 12) if mode != 'OFF' else 0x0000
        self.log.trace("Writing register {:02X} with {:04X}".format(addr, data_w))
        self.regs.poke16(addr, data_w)
        # 5. config62, make sure bits 12:11 are set to 0x0 to disable character alignment.
        addr = 0x3E
        rb = self.regs.peek16(addr)
        if ((rb & 0x58) >> 11) != 0x0:
            self.log.error("Char alignment is enabled when not expected.")
