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
TDC clock synchronization
"""

import time
from builtins import object
from functools import reduce

def mean(vals):
    " Calculate arithmetic mean of vals "
    return float(sum(vals)) / max(len(vals), 1)

def rsp_table(ref_clk_freq, radio_clk_freq):
    """
    For synchronization: Returns RTC values. In NI language, these are
    kRspPeriodInRClks and kRspHighTimeInRClks.

    Returns a tuple (period, high_time).
    """
    return {
        125e6: {
            10e6: (10, 5),
            20e6: (20, 10),
            25e6: (25, 13),
        },
        122.8e6: {
            10e6: (250, 125),
            20e6: (500, 250),
            25e6: (625, 313),
        },
        156.3e6: {
            10e6: (250, 125),
            20e6: (500, 250),
            25e6: (625, 313),
        },
        104e6: {
            10e6: (10, 5),
            20e6: (20, 10),
            25e6: (25, 13),
        },
    }[radio_clk_freq][ref_clk_freq]

def rtc_table(radio_clk_freq):
    """
    For synchronization: Returns RTC values. In NI language, these are
    kRtcPeriodInSClks and kRtcHighTimeInSClks.

    Returns a tuple (period, high_time).
    """
    return {
        125e6:   (125, 62.5),
        122.8e6: (3072, 1536),
        156.3e6: (3840, 1920),
        104e6:   (104, 52),
    }[radio_clk_freq]


class ClockSynchronizer(object):
    """
    Runs the clock synchronization routine for the daughterboard. Sets up and then
    retrieves the measurement from the TDC in the FPGA and adjusts the LMK clocks
    as needed to compensate.

    The actual synchronization is run in run_sync().
    """
    # TDC Control Register address constants
    # TODO see if we want to factor in an offset
    TDC_CONTROL         = 0x200
    TDC_STATUS          = 0x208
    RSP_OFFSET_0        = 0x20C
    RSP_OFFSET_1        = 0x210
    RTC_OFFSET_0        = 0x214
    RTC_OFFSET_1        = 0x218
    RSP_PERIOD_CONTROL  = 0x220
    RTC_PERIOD_CONTROL  = 0x224

    def __init__(
            self,
            regs_iface,
            dboard_clk_control,
            lmk,
            phase_dac,
            offset,
            radio_clk_freq,
            ref_clk_freq,
            fine_delay_step,
            init_pdac_word,
            log
        ):
        self._iface = regs_iface
        self.log = log
        self.peek32 = lambda addr: self._iface.peek32(addr + offset)
        self.poke32 = lambda addr, data: self._iface.poke32(addr + offset, data)
        self.lmk = lmk
        self.phase_dac = phase_dac
        self.dboard_clk_control = dboard_clk_control
        self.radio_clk_freq = radio_clk_freq
        self.ref_clk_freq = ref_clk_freq
        self.fine_delay_step = fine_delay_step
        self.current_phase_dac_word = init_pdac_word

    def run_sync(self, measurement_only=False):
        """
        Perform the synchronization algorithm. Successful completion of this
        function means the clock output was synchronized to the reference.

        - Set RTC and RSP values in synchronization core
        - Run offset measurements
        - Calcuate LMK shift and phase DAC values from offsets
        - Check it all worked

        """
        # To access registers, use self.peek32() and self.poke32(). It already contains
        # the offset at this point (see __init__), so self.peek32(0x0000) should read the
        # first offset if you kept your reg offset at 0 in your netlist

        self.log.trace("Running clock synchronization...")

        # Reset and disable TDC, disable PPS crossing, and enable re-runs. Confirm the
        # core is in reset and PPS is cleared.
        self.poke32(self.TDC_CONTROL, 0x2121)
        reset_status = self.peek32(self.TDC_STATUS) & 0xFF
        if reset_status != 0x01:
            self.log.error("TDC Failed to Reset! Status: 0x{:x}".format(reset_status))
            raise RuntimeError("TDC Failed to reset.")

        # Set the RSP and RTC values based on the Radio Clock and Reference Clock
        # configurations. Registers are packed [27:16] = high time, [11:0] = period.
        def combine_period_hi_time(period, hi_time):
            """
            Registers are packed [27:16] = high time, [11:0] = period.
            """
            assert hi_time <= 0xFFF and period <= 0xFFF
            return (hi_time << 16) | period
        rsp_ctrl_word = combine_period_hi_time(
            *rsp_table(self.ref_clk_freq, self.radio_clk_freq)
        )
        rtc_ctrl_word = combine_period_hi_time(
            *rtc_table(self.radio_clk_freq)
        )
        self.log.trace("Setting RSP control word to: 0x{:08X}".format(rsp_ctrl_word))
        self.log.trace("Setting RTC control word to: 0x{:08X}".format(rtc_ctrl_word))
        self.poke32(self.RSP_PERIOD_CONTROL, rsp_ctrl_word)
        self.poke32(self.RTC_PERIOD_CONTROL, rtc_ctrl_word)

        # Take the core out of reset, then check the reset done bit cleared.
        self.poke32(self.TDC_CONTROL, 0x2)
        reset_status = self.peek32(self.TDC_STATUS) & 0xFF
        if reset_status != 0x00:
            self.log.error(
                "TDC Reset Failed to Clear! " \
                "Check that your clocks are toggling. Status: 0x{:x}".format(reset_status)
            )
            raise RuntimeError("TDC Reset Failed.")
        
        
        self.log.trace("Enabling the TDC")
        # Enable the TDC.
        # As long as PPS is actually a PPS, this doesn't have to happen "synchronously"
        # across all devices.
        self.poke32(self.TDC_CONTROL, 0x10)

        # Since a PPS rising edge comes once per second... we only need to wait slightly
        # longer than a second to confirm the TDC received a PPS.
        # TODO change this to a polling operation
        time.sleep(1.1)
        pps_status = self.peek32(self.TDC_STATUS) & 0xFF
        if pps_status != 0x10:
            self.log.error("Failed to capture PPS within 1.1 seconds. Status: 0x{:x}".format(pps_status))
            raise RuntimeError("Failed to capture PPS.")

        self.log.trace("PPS Captured!")
        
        meas_clk_freq = 170.542641116e6
        measure_offset = lambda: self.read_tdc_meas(
            1/meas_clk_freq, 1/self.ref_clk_freq, 1/self.radio_clk_freq
        )
        # Retrieve the first measurement, but throw it away since it won't align with
        # all the re-run measurements.
        self.log.trace("Throwing away first TDC measurement...")
        measure_offset()

        # Now, read off 512 measurements and take the mean of them.
        num_meas = 256 # FIXME back to 512
        self.log.trace("Reading {} TDC measurements from device...".format(num_meas))
        current_value = mean([measure_offset() for _ in range(num_meas)])

        # Run the initial value through the oracle to determine the adjustments to make.
        target_values = [135e-9,] # only one target for now that all DBs shift to
        lmk_vco_freq = 2.496e9    # LMK VCO = 2.496 GHz
        coarse_steps_required, dac_word_delta, distance_to_target = self.oracle(
            target_values,
            current_value,
            lmk_vco_freq,
            self.fine_delay_step
        )

        if not measurement_only:
            self.log.trace("Applying calculated shifts...")
            self.dboard_clk_control.enable_outputs(False)
            # Coarse shift with the LMK.
            self.lmk.lmk_shift(coarse_steps_required)
            self.log.trace("LMK Shift Complete!")
            # Fine shift with the DAC, then give it time to take effect.
            self.current_phase_dac_word = (self.current_phase_dac_word + dac_word_delta) & 0xFFF
            self.log.trace("Writing Phase DAC Word: {}".format(self.current_phase_dac_word))
            self.phase_dac.poke16(0x3, self.current_phase_dac_word)
            time.sleep(0.5)
            if not self.lmk.check_plls_locked():
                raise RuntimeError("LMK PLLs lost lock during clock synchronization!")
            self.dboard_clk_control.enable_outputs(True)
        return distance_to_target


    def read_tdc_meas(
            self,
            meas_clk_period=1/170.542641116e6,
            ref_clk_period=1/10e6,
            radio_clk_period=1/104e6
        ):
        """
        Return the offset (in seconds) the whatever what measured and whatever
        the reference is.
        """
        for _ in range(1000): # TODO replace with poll & timeout
            rtc_offset_msb = self.peek32(self.RTC_OFFSET_1)
            updated = (rtc_offset_msb & 0x100) == 0x100
            if updated:
                break
            time.sleep(0.001)
        if not updated:
            self.log.error("Offsets failed to update within timeout.")
            raise RuntimeError("Offsets failed to update within timeout.")

        rtc_offset = (rtc_offset_msb & 0xFF) << 32
        rtc_offset = float(rtc_offset | self.peek32(self.RTC_OFFSET_0)) / (1<<27)

        rsp_offset = (self.peek32(self.RSP_OFFSET_1) & 0xFF) << 32
        rsp_offset = float(rsp_offset | self.peek32(self.RSP_OFFSET_0)) / (1<<27)

        offset = (rtc_offset - rsp_offset)*meas_clk_period + ref_clk_period - radio_clk_period

        return offset



    def oracle(self, target_values, current_value, lmk_vco_freq, fine_delay_step):
        """
        target_values -- The desired offset (seconds). Can be a list of values,
                         in which case the target value that is closest to the
                         current value is chosen.
        current_value -- Current offset (seconds)
        current_dac_word -- Current DAC word
        lmk_vco_freq -- LMK VCO frequency. We can shift using the LMK in steps
                        of 1/lmk_vco_freq

        Returns a tuple with the number of LMK coarse delay steps and the delta
        that needs to be applied to the DAC output word.

        Description of algorithm:
        1) Out of the given target values, find the one that's closest to the
           current value.
        2) Figure out the number how many steps the LMK needs to shift by for
           a coarse correction
        3) Calculate DAC word for shifting the rest
        """
        target_value = reduce(
            lambda a, x: a if (abs(a-current_value) < abs(x-current_value)) else x,
            target_values
        )
        distance_to_target = target_value - current_value
        self.log.trace("Target value = {} ns. Current value = {} ns. Distance to target = {} ns.".format(
            target_value*1e9,
            current_value*1e9,
            distance_to_target*1e9,
        ))
        # Determine the sign.
        sign = 1 if distance_to_target >= 0 else -1
        coarse_step_size = 1/lmk_vco_freq
        # For negative input values, divmod occasionally returns coarse steps -1 from
        # the correct value. To combat this blatent crime, I just give it a positive value
        # and then sign-correct afterwards.
        coarse_steps_required, remainder = divmod(abs(distance_to_target), coarse_step_size)
        coarse_steps_required = int(coarse_steps_required * sign)
        self.log.trace("Coarse Steps (LMK): {}".format(coarse_steps_required))
        dac_word_delta = int(remainder // fine_delay_step) * sign
        self.log.trace("Fine Steps (DAC):   {}".format(dac_word_delta))
        return coarse_steps_required, dac_word_delta, distance_to_target

