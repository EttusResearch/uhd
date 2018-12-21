#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
AD9695 driver for use with Rhodium
"""

import time
from builtins import object
from ..mpmlog import get_logger

class AD9695Rh(object):
    """
    This class provides an interface to configure the AD9695 IC through SPI.
    """

    ADC_CHIP_ID = 0xDE

    CHIP_CONFIGURATION_REG = 0x0002
    CHIP_ID_LSB_REG        = 0x0004
    SCRATCH_PAD_REG        = 0x000A

    def __init__(self, slot_idx, regs_iface, parent_log=None):
        self.log = parent_log.getChild("AD9695") if parent_log is not None \
            else get_logger("AD9695-{}".format(slot_idx))
        self.regs = regs_iface
        assert hasattr(self.regs, 'peek8')
        assert hasattr(self.regs, 'poke8')

        def _verify_chip_id():
            chip_id = self.regs.peek8(self.CHIP_ID_LSB_REG)
            self.log.trace("ADC Chip ID: 0x{:X}".format(chip_id))
            if chip_id != self.ADC_CHIP_ID:
                self.log.error("Wrong Chip ID 0x{:X}".format(chip_id))
                return False
            return True

        if not _verify_chip_id():
            raise RuntimeError("Unable to locate AD9695")


    def assert_scratch(self, scratch_val=0xAD):
        """
        Method that validates the scratch register by poking and peeking.
        """
        self.regs.poke8(self.SCRATCH_PAD_REG, scratch_val)
        self.log.trace("Scratch write value: 0x{:X}".format(scratch_val))
        scratch_rb = self.regs.peek8(self.SCRATCH_PAD_REG) & 0xFF
        self.log.trace("Scratch readback: 0x{:X}".format(scratch_rb))
        if scratch_rb != scratch_val:
            raise RuntimeError("Wrong ADC scratch readback: 0x{:X}".format(scratch_rb))

    def pokes8(self, addr_vals):
        """
        Apply a series of pokes.
        pokes8((0,1),(0,2)) is the same as calling poke8(0,1), poke8(0,2).
        """
        for addr, val in addr_vals:
            self.regs.poke8(addr, val)


    def power_down_channel(self, power_down=False):
        """
        This method either powers up/down the channel according to register 0x0002 [1:0]:
          power_down = True  -> Power-down mode.
                                  Digital datapath clocks disabled; digital datapath held
                                  in reset; JESD204B interface disabled.
          power_down = False -> Normal mode.
                                  Channel powered up.
        """
        power_mode = 0b11 if power_down else 0b00
        self.regs.poke8(self.CHIP_CONFIGURATION_REG, power_mode)


    def init(self):
        """
        Basic init that resets the ADC and verifies it.
        """
        self.power_down_channel(False) # Power-up the channel.
        self.log.trace("Reset ADC & Verify")
        self.regs.poke8(0x0000, 0x81) # Soft-reset the ADC (self-clearing).
        time.sleep(0.005)             # We must allow 5 ms for the ADC's bootloader.
        self.assert_scratch(0xAD)     # Verify scratch register R/W access.
        self.regs.poke8(0x0571, 0x15) # Powerdown the JESD204B serial transmit link.
        self.log.trace("ADC's JESD204B link powered down.")


    def config(self):
        """
        Check the clock status, and write configuration values!
        Before performing the above, the chip is soft-reset through the
        serial interface.
        """
        self.init()

        clock_status = self.regs.peek8(0x011B) & 0xFF
        self.log.trace("Clock status readback: 0x{:X}".format(clock_status))
        if clock_status != 0x01:
            self.log.error("Input clock not detected")
            raise RuntimeError("Input clock not detected for ADC")

        self.log.trace("ADC Configuration.")
        self.pokes8((
            (0x003F, 0x80), # Disable PDWN/STBY pin.
            (0x0040, 0x00), # FD_A|B pins configured as Fast Detect outputs.
            (0x0559, 0x11), # Configure tail bits as overrange bits (1:0) (Based on VST2).
            (0x055A, 0x01), # Configure tail bits as overrange bits (2) (Based on VST2).
            (0x058D, 0x17), # Set K = d23 (0x17) (Frames per multiframe).
            (0x0550, 0x00), # Test mode pattern generation: OFF.
            (0x0571, 0x15), # JESD Link mode: ILA enabled with K28.3 and K28.7; link powered down.
            (0x058F, 0x0D), # CS = 0 (Control bits); N = 14 (ADC resolution).
            (0x056E, 0x10), # JESD204B lane rate range: 3.375 Gbps to 6.75 Gbps.
            (0x058B, 0x03), # Scrambling disabled; L = 4 (number of lanes).
            (0x058C, 0x00), # F = 1 (0x0 + 1) (Number of octets per frame)
            (0x058E, 0x01), # M = 2 (0x1) (Number of converters per link).
            (0x05B2, 0x01), # SERDOUT0 mapped to lane 1.
            (0x05B3, 0x00), # SERDOUT1 mapped to lane 0.
            (0x05C0, 0x10), # SERDOUT0 voltage swing adjust (improves RX margin at FPGA).
            (0x05C1, 0x10), # SERDOUT1 voltage swing adjust (improves RX margin at FPGA).
            (0x05C2, 0x10), # SERDOUT2 voltage swing adjust (improves RX margin at FPGA).
            (0x05C3, 0x10), # SERDOUT3 voltage swing adjust (improves RX margin at FPGA).
        ))
        self.log.trace("ADC register dump finished.")


    def init_framer(self):
        """
        Initialize the ADC's framer, and check the PLL for lock.
        """
        def _check_pll_lock():
            pll_lock_status = self.regs.peek8(0x056F)
            if (pll_lock_status & 0x88) != 0x80:
                self.log.debug("PLL reporting unlocked... Status: 0x{:x}"
                               .format(pll_lock_status))
                return False
            return True

        self.log.trace("Initializing framer...")
        self.pokes8((
            (0x0571, 0x14), # Powerup the link before JESD204B initialization.
            (0x1228, 0x4F), # Reset JESD204B start-up circuit
            (0x1228, 0x0F), # JESD204B start-up circuit in normal operation. The value may be 0x00.
            (0x1222, 0x00), # JESD204B PLL force normal operation
            (0x1222, 0x04), # Reset JESD204B PLL calibration
            (0x1222, 0x00), # JESD204B PLL normal operation
            (0x1262, 0x08), # Clear loss of lock bit
            (0x1262, 0x00), # Loss of lock bit normal operation
        ))

        self.log.trace("Polling for PLL lock...")
        locked = False
        for _ in range(6):
            time.sleep(0.001)
            # Clear stickies possibly?
            if _check_pll_lock():
                locked = True
                self.log.info("ADC PLL Locked!")
                break
        if not locked:
            raise RuntimeError("ADC PLL did not lock! Check the logs for details.")

        self.log.trace("ADC framer initialized.")

    def enable_sysref_capture(self, enabled=False):
        """
        Enable the SYSREF capture block.
        """
        sysref_ctl1 = 0x00 # Default value is disabled.
        if enabled:
            sysref_ctl1 = 0b1 << 2 # N-Shot SYSREF mode

        self.log.trace("%s ADC SYSREF capture..." % {True: 'Enabling', False: 'Disabling'}[enabled])
        self.pokes8((
            (0x0120, sysref_ctl1), # Capture low-to-high N-shot SYSREF transitions on CLK's RE
            (0x0121, 0x00),        # Capture the next SYSREF only.
        ))
        self.log.trace("ADC SYSREF capture %s." % {True: 'enabled', False: 'disabled'}[enabled])


    def check_framer_status(self):
        """
        This function checks the status of the framer by checking SYSREF capture regs.
        """
        SYSREF_MONITOR_MESSAGES = {
            0 : "Condition not defined!",
            1 : "Possible setup error. The smaller the setup, the smaller its margin.",
            2 : "No setup or hold error (best hold margin).",
            3 : "No setup or hold error (best setup and hold margin).",
            4 : "No setup or hold error (best setup margin).",
            5 : "Possible hold error. The largest the hold, the smaller its margin.",
            6 : "Possible setup or hold error."
        }

        # This is based of Table 37 in the AD9695's datasheet.
        def _decode_setup_hold(setup, hold):
            status = 0
            if setup == 0x0:
                if hold == 0x0:
                    status = 6
                elif hold == 0x8:
                    status = 4
                elif (hold >= 0x9) and (hold <= 0xF):
                    status = 5
            elif (setup <= 0x7) and (hold == 0x0):
                status = 1
            elif (setup == 0x8) and ((hold >= 0x0) and (hold <= 0x8)):
                status = 2
            elif hold == 0x8:
                status = 3
            return status

        self.log.trace("Checking ADC's framer status.")

        # Read the SYSREF setup and hold monitor register.
        sysref_setup_hold_monitor = self.regs.peek8(0x0128)
        sysref_setup = sysref_setup_hold_monitor & 0x0F
        sysref_hold = (sysref_setup_hold_monitor & 0xF0) >> 4
        sysref_monitor_status = _decode_setup_hold(sysref_setup, sysref_hold)
        self.log.trace("SYSREF setup: 0x{:X}".format(sysref_setup))
        self.log.trace("SYSREF hold: 0x{:X}".format(sysref_hold))
        self.log.debug("SYSREF monitor: %s" % SYSREF_MONITOR_MESSAGES[sysref_monitor_status])

        # Read the Clock divider phase when SYSREF is captured.
        sysref_phase = self.regs.peek8(0x0129) & 0x0F
        self.log.trace("SYSREF capture was %.2f cycle(s) delayed from clock.", (sysref_phase * 0.5))
        self.log.trace("Clk divider phase when SYSREF was captured: 0x{:X}".format(sysref_phase))

        # Read the SYSREF counter.
        sysref_count = self.regs.peek8(0x012A) & 0xFF
        self.log.trace("%d SYSREF events were captured." % sysref_count)

        if sysref_count == 0x0:
            self.log.error("A SYSREF event was not captured by the ADC.")
        elif sysref_monitor_status == 0:
            self.log.trace("The SYSREF setup & hold monitor status is not defined.")
        elif sysref_monitor_status in (1, 5, 6):
            self.log.warning("SYSREF monitor: %s" % SYSREF_MONITOR_MESSAGES[sysref_monitor_status])
        elif sysref_phase > 0x0:
            self.log.trace("SYSREF capture was %.2f cycle(s) delayed from clock." % (sysref_phase * 0.5))
        return sysref_count >= 0x01
