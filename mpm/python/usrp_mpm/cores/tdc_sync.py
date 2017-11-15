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
import math
from builtins import object
from functools import reduce
from usrp_mpm.mpmutils import poll_with_timeout

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
        122.88e6: {
            10e6: (250, 125),
            20e6: (500, 250),
            25e6: (625, 313),
        },
        153.6e6: {
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
        125e6:    (125,  63),
        122.88e6: (3072, 1536),
        153.6e6:  (3840, 1920),
        104e6:    (104,  52),
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
            lmk,
            phase_dac,
            offset,
            radio_clk_freq,
            ref_clk_freq,
            fine_delay_step,
            init_pdac_word,
            target_values,
            dac_spi_addr_val,
            log
        ):
        self._iface = regs_iface
        self.log = log
        self.peek32 = lambda addr: self._iface.peek32(addr + offset)
        self.poke32 = lambda addr, data: self._iface.poke32(addr + offset, data)
        self.lmk = lmk
        self.phase_dac = phase_dac
        self.radio_clk_freq = radio_clk_freq
        self.ref_clk_freq = ref_clk_freq
        self.fine_delay_step = fine_delay_step
        self.current_phase_dac_word = init_pdac_word
        self.lmk_vco_freq = self.lmk.get_vco_freq()
        self.target_values = target_values
        self.dac_spi_addr_val = dac_spi_addr_val
        self.meas_clk_freq = 170.542641116e6

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

        # Reset and disable TDC, and enable re-runs. Confirm the core is in
        # reset and PPS is cleared. Do not disable the PPS crossing.
        self.poke32(self.TDC_CONTROL, 0x0121)
        reset_status = self.peek32(self.TDC_STATUS) & 0xFF
        if reset_status != 0x01:
            self.log.error("TDC Failed to Reset! Status: 0x{:x}".format(
                reset_status
            ))
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
                "Check that your clocks are toggling. Status: 0x{:x}".format(
                    reset_status
            ))
            raise RuntimeError("TDC Reset Failed.")
        self.log.trace("Enabling the TDC")
        # Enable the TDC.
        # As long as PPS is actually a PPS, this doesn't have to happen "synchronously"
        # across all devices.
        self.poke32(self.TDC_CONTROL, 0x10)

        # Since a PPS rising edge comes once per second, we need to wait
        # slightly longer than a second (worst-case) to confirm the TDC
        # received a PPS.
        if not poll_with_timeout(
                lambda: bool(self.peek32(self.TDC_STATUS) & 0xFF != 0x10),
                1100, # Try for 1.1 seconds
                100, # Poll every 100 ms
            ):
            self.log.error("Failed to capture PPS within 1.1 seconds. " \
                           "TDC_STATUS: 0x{:X}".format(pps_status))
            raise RuntimeError("Failed to capture PPS.")
        self.log.trace("PPS Captured!")

        measure_offset = lambda: self.read_tdc_meas(
            1.0/self.meas_clk_freq, 1.0/self.ref_clk_freq, 1.0/self.radio_clk_freq
        )
        # Retrieve the first measurement, but throw it away since it won't align with
        # all the re-run measurements.
        self.log.trace("Throwing away first TDC measurement...")
        measure_offset()

        # Now, read off 512 measurements and take the mean of them.
        num_meas = 256
        self.log.trace("Reading {} TDC measurements from device...".format(num_meas))
        current_value = mean([measure_offset() for _ in range(num_meas)])
        self.log.trace("TDC measurements collected.")

        # The high and low bounds for this are set programmatically based on the
        # Reference and Sample Frequencies and the TDC structure. The bounds are:
        # Low  = T_refclk + T_sampleclk*(3)
        # High = T_refclk + T_sampleclk*(4)
        # For slop, we add in another T_sampleclk on either side.
        low_bound = 1.0/self.ref_clk_freq + (1.0/self.radio_clk_freq)*2
        high_bound = 1.0/self.ref_clk_freq + (1.0/self.radio_clk_freq)*5
        if (current_value < low_bound) or (current_value > high_bound):
            self.log.error("Clock synchronizer measured a "
                           "current value of {:.3f} ns. " \
                           "Range is [{:.3f},{:.3f}] ns".format(
                               current_value*1e9,
                               low_bound*1e9,
                               high_bound*1e9))
            raise RuntimeError("TDC measurement out of range! "
                               "Current value: {:.3f} ns.".format(
                                   current_value*1e9))

        # Run the initial value through the oracle to determine the adjustments to make.
        coarse_steps_required, dac_word_delta, distance_to_target = self.oracle(
            self.target_values,
            current_value,
            self.lmk_vco_freq,
            self.fine_delay_step
        )

        # Check the calculated distance_to_target value. It should be less than
        # +/- 1 radio_clk_freq period. The boundary values are set using the same
        # logic as the high and low bound checks above on the current_value.
        if abs(distance_to_target) > 1.0/self.radio_clk_freq:
            self.log.error("Clock synchronizer measured a "
                           "distance to target of {:.3f} ns. " \
                           "Range is [{:.3f},{:.3f}] ns".format(
                               distance_to_target*1e9,
                               -1.0/self.radio_clk_freq*1e9,
                               1.0/self.radio_clk_freq*1e9))
            raise RuntimeError("TDC measured distance to target is out of range! "
                               "Current value: {:.3f} ns.".format(
                                   distance_to_target*1e9))

        if not measurement_only:
            self.log.trace("Applying calculated shifts...")
            # Coarse shift with the LMK.
            self.lmk.lmk_shift(coarse_steps_required)
            self.log.trace("LMK Shift Complete!")
            # Fine shift with the DAC, then give it time to take effect.
            self.write_dac_word(self.current_phase_dac_word + dac_word_delta, 0.5)
            if not self.lmk.check_plls_locked():
                raise RuntimeError("LMK PLLs lost lock during clock synchronization!")
            # After shifting the clocks, we enable the PPS crossing from the
            # RefClk into the SampleClk domain. We never explicitly turn off the
            # crossing from this point forward, even if we re-run this routine.
            self.poke32(self.TDC_CONTROL, 0x1000)

        return distance_to_target


    def read_tdc_meas(
            self,
            meas_clk_period=1.0/170.542641116e6,
            ref_clk_period=1.0/10e6,
            radio_clk_period=1.0/104e6
        ):
        """
        Return the offset (in seconds) the whatever what measured and whatever
        the reference is.
        """
        # Current worst-case time is around 3.5s.
        timeout = time.time() + 4.0 # TODO knock this back down after optimizations
        while True:
            rtc_offset_msb = self.peek32(self.RTC_OFFSET_1)
            if rtc_offset_msb & 0x100 == 0x100:
                break
            if time.time() > timeout:
                error_msg = "Offsets failed to update within timeout."
                self.log.error(error_msg)
                raise RuntimeError(error_msg)

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
        self.log.trace("Target value = {:.3f} ns. Current value = {:.3f} ns. Distance to target = {:.3f} ns.".format(
            target_value*1e9,
            current_value*1e9,
            distance_to_target*1e9,
        ))
        # Determine the sign.
        sign = 1 if distance_to_target >= 0 else -1
        coarse_step_size = 1.0/lmk_vco_freq
        # For negative input values, divmod occasionally returns coarse steps -1 from
        # the correct value. To combat this blatent crime, I just give it a positive value
        # and then sign-correct afterwards.
        coarse_steps_required, remainder = divmod(abs(distance_to_target), coarse_step_size)
        coarse_steps_required = int(coarse_steps_required * sign)
        self.log.trace("Coarse Steps (LMK): {}".format(coarse_steps_required))
        dac_word_delta = int(remainder // fine_delay_step) * sign
        self.log.trace("Fine Steps (DAC):   {}".format(dac_word_delta))
        return coarse_steps_required, dac_word_delta, distance_to_target


    def write_dac_word(self, word, settling_time=1.0):
        """
        Write the word and wait for settling time.
        TODO: hard-coded to support 16 bit DACs. do we need to modify to support smaller?
        """
        self.current_phase_dac_word = word & 0xFFFF
        self.log.trace("Writing Phase DAC Word: {}".format(self.current_phase_dac_word))
        self.phase_dac.poke16(self.dac_spi_addr_val, self.current_phase_dac_word)
        time.sleep(settling_time) # settling time.
        return self.current_phase_dac_word


    def test_dac_flatness(self, low_bound, high_bound, middle_samples):
        """
        Take several TDC measurements using DAC settings from [low_bound, high_bound].
        Minimum number of measurements is 2, for the low and high bounds, and
        middle_samples defines the number of measurements taken between the bounds.

        Writes these samples along with their corresponding DAC values to a file for
        post-processing.
        """
        results = []

        self.log.trace("Starting DAC Flatness Test...")
        self.log.trace("DAC Low Bound: {}. DAC High Bound: {}".format(low_bound, high_bound))

        # open file for writing
        meas_file = open("dac_flatness_results.txt", "w")
        meas_file.write("DAC Word, Distance to Target (ns)\n")

        inc = math.floor((high_bound - low_bound)/(middle_samples + 1.0))

        for x in range(middle_samples + 2):
            self.log.info("Test Progress: {:.2f}%".format(x*100/(middle_samples+2)))
            self.write_dac_word(x*inc + low_bound, 0.1)
            distance_to_target = self.run_sync(measurement_only=True)
            meas_file.write("{}, {:.4f}\n".format(x*inc + low_bound, distance_to_target*1e12))
            results.append(distance_to_target*1e12)

        meas_file.close()

        self.log.info("Results:")
        for y in range(middle_samples + 2):
            self.log.info("Word: {}   Measurement: {:.6f}".format(y*inc + low_bound, results[y]))

        return True


    def dac_bist(self, taps_from_center = 100):
        """
        A quick BIST of the DAC, proving it is (a) alive and (b) can shift the clock.

        Instructions:
        - Replace the <_sync_db_clock(self.clock_synchronizer)> call in the dboard
          manager with <self.clock_synchronizer.dac_bist(100)>, replacing 100 with
          the desired taps_from_center value.
        - Run MPM with: # usrp_hwd.py --init-only

        The test performs 3 TDC measurements with three DAC settings:
          1) at the initial DAC setting
          2) below the initial setting
          3) above the initial setting
        The following checks are performed on the resulting measurments:
          1) they are monotonicly increasing in this order: 2, 1, 3
          2) the distance from 2 -> 1 and 2 -> 3 is within some range based on calculated
             expected values
        """
        test1_result = False
        test2_result = False

        self.log.info("Starting Phase DAC BIST Test...")

        # We need to move the clock to a mid-range value to avoid wrap-around effects
        # on our measurements below. Even if the PDAC isn't functional, this sync run
        # will use the LMK digital delays to bring our clocks well within the required
        # ranges.
        self.log.trace("Running initial synchronization as a precursor before BIST:")
        self.run_sync(measurement_only=False)

        # Take a measurement at the current value (which should be near our target value).
        center_meas = self.target_values[0] + self.run_sync(measurement_only=True)

        # Modify the DAC word to below the default value, then above, repeating the
        # measurements each time.
        self.write_dac_word(self.current_phase_dac_word + taps_from_center, 0.5)
        low_meas = self.target_values[0] + self.run_sync(measurement_only=True)
        self.write_dac_word(self.current_phase_dac_word - 2*taps_from_center, 0.5)
        high_meas = self.target_values[0] + self.run_sync(measurement_only=True)

        self.log.info("Phase DAC BIST Raw Results:")
        self.log.info("Low Measurement:    {:.6f} ns".format(low_meas*1e9))
        self.log.info("Center Measurement: {:.6f} ns".format(center_meas*1e9))
        self.log.info("High Measurement:   {:.6f} ns".format(high_meas*1e9))

        # Test #1: Are all the measurements increasing: low -> center -> high?
        if (high_meas > center_meas) and (center_meas > low_meas):
            test1_result = True
        else:
            self.log.warning("DAC BIST values are not monotonically increasing!")

        # Test #2: Are the measurements spaced appropriately?
        expected_meas = taps_from_center*self.fine_delay_step
        actual_high_meas = high_meas - center_meas
        actual_low_meas = center_meas - low_meas

        # TDC measurement error is typically 40 ps. We should expected that the variation
        # between measurements is significantly less than this, but still prepare for the
        # worst... therefore we check to ensure our expected measurement spread is
        # higher than our expected error.
        allowable_error = 40e-12
        if expected_meas < allowable_error:
            self.log.warning("Expected measurement offset is less than the allowable error \
of {:.2f} ps. Consider increasing the taps_from_center parameter." \
                           .format(allowable_error*1e12))

        if  abs(actual_high_meas - expected_meas) < allowable_error and \
            abs(actual_low_meas  - expected_meas) < allowable_error:
            test2_result = True
        else:
            self.log.warning("DAC BIST values are not spaced correctly!")

        if not(test1_result and test2_result):
            raise RuntimeError("Phase DAC BIST Failed!")

        self.log.info("Phase DAC BIST Success!")
        return True

