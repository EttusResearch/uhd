#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
TDC clock synchronization
"""

import time
import math
from fractions import gcd
from functools import reduce
from builtins import object
from usrp_mpm.mpmutils import poll_with_timeout
from usrp_mpm.mpmlog import get_logger

def mean(vals):
    " Calculate arithmetic mean of vals "
    return float(sum(vals)) / max(len(vals), 1)


class ClockSynchronizer(object):
    """
    Runs the clock synchronization routine for the daughterboard. Sets up and then
    retrieves the measurement from the TDC in the FPGA and adjusts the LMK clocks
    as needed to compensate.

    The actual synchronization is run in run_sync().
    """
    # TDC Control Register address constants
    TDC_CONTROL              = 0x000
    TDC_STATUS               = 0x008
    RP_OFFSET_0              = 0x00C
    RP_OFFSET_1              = 0x010
    SP_OFFSET_0              = 0x014
    SP_OFFSET_1              = 0x018
    RP_PERIOD_CONTROL        = 0x020
    SP_PERIOD_CONTROL        = 0x024
    RPT_PERIOD_CONTROL       = 0x028
    SPT_PERIOD_CONTROL       = 0x02C
    TDC_MASTER_RESET         = 0x030
    REPULSE_PERIOD_CONTROL_1 = 0x040
    REPULSE_PERIOD_CONTROL_2 = 0x044
    SYNC_SIGNATURE           = 0x100
    SYNC_REVISION            = 0x104
    SYNC_OLDESTCOMPAT        = 0x108
    SYNC_SCRATCH             = 0x10C

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
            dac_spi_addr_val,
            pps_in_pipe_ext_delay,
            pps_in_pipe_dynamic_delay,
            slot_idx
        ):
        self._iface = regs_iface
        self.log = get_logger("Sync-{}".format(slot_idx))
        self.slot_idx = slot_idx
        self.peek32 = lambda addr: self._iface.peek32(addr + offset)
        self.poke32 = lambda addr, data: self._iface.poke32(addr + offset, data)
        self.lmk = lmk
        self.phase_dac = phase_dac
        self.radio_clk_freq = radio_clk_freq
        self.ref_clk_freq = ref_clk_freq
        self.fine_delay_step = fine_delay_step
        self.current_phase_dac_word = init_pdac_word
        self.lmk_vco_freq = self.lmk.get_vco_freq()
        self.dac_spi_addr_val = dac_spi_addr_val
        self.meas_clk_freq = None
        self.target_values = []
        # Output PPS static delay is the minimum number of radio_clk cycles from the SP-t
        # rising edge to when PPS appears on the output of the trigger passing module in
        # the radio_clk domain. 2 cycles are from the trigger crossing structure and
        # 2 are from the double-synchronizer that crosses the PPS output into the
        # no-reset domain from the async reset domain of the TDC.
        self.PPS_OUT_PIPE_STATIC_DELAY = 2+2
        # Output PPS variable delay is programmable by this module to between 0 and 15
        # radio_clk cycles. The combination of static and variable delays make up the
        # total delay from SP-t rising edge to the PPS in the radio_clk domain.
        self.pps_out_pipe_var_delay = 0
        # Input PPS delay (in ref_clk cycles) is recorded here and only changes when
        # the TDC structure changes. This represents the number of ref_clk cycles from
        # PPS arriving at the input of the TDC to when the RP/-t pulse occurs. There are
        # static delays and dynamic delays. Dynamic delay is 0-15 additional cycles.
        # Default for older drivers was a total of 5 cycles. Increase the dynamic delay
        # to delay the RP/SP pulsers start with respect to the Reset Pulser.
        self.PPS_IN_PIPE_STATIC_DELAY = 4
        self.pps_in_pipe_dynamic_delay = pps_in_pipe_dynamic_delay # 0-15
        # External input PPS delay is a target-specific value, typically 3 ref_clk cycles.
        # This represents the number of ref_clk cycles from when PPS is first captured
        # by the ref_clk to when PPS arrives at the input of the TDC.
        self.pps_in_pipe_ext_delay = pps_in_pipe_ext_delay
        self.tdc_rev = 1
        # update theses lists whenever more rates are supported
        self.SUPPORTED_PULSE_RATES = [1e6, 1.25e6, 1.2288e6] # order matters here!
        self.SUPPORTED_REF_CLK_FREQS = [10e6, 20e6, 25e6, 62.5e6]
        if self.ref_clk_freq not in self.SUPPORTED_REF_CLK_FREQS:
            self.log.error("Clock synchronizer does not support the selected reference "
                           "clock frequency. Selected rate: {:.2f} MHz".format(
                               self.ref_clk_freq*1e-6))
            raise RuntimeError("TDC does not support the selected reference clock rate!")
        self.supported_radio_clk_freqs = [104e6, 122.88e6, 125e6, 153.6e6, 156.25e6, \
                                          200e6, 250e6]
        if self.radio_clk_freq not in self.supported_radio_clk_freqs:
            self.log.error("Clock synchronizer does not support the selected radio clock"
                           " frequency. Selected rate: {:.2f} MHz".format(
                               self.radio_clk_freq*1e-6))
            raise RuntimeError("TDC does not support the selected radio clock rate!")

        # Bump this whenever we stop supporting older FPGA images or boards.
        # YYMMDDHH
        self.oldest_compat_version = 0x17060111
        # Bump this whenever changes are made to this MPM host code.
        self.current_version = 0x18032916
        self.check_core()
        self.configured = False


    def check_core(self):
        """
        Verify TDC core returns correct ID and passes revision tests.
        """
        self.log.trace("Checking TDC Core...")
        if self.peek32(self.SYNC_SIGNATURE) != 0x73796e63: # SYNC in ASCII hex
            raise RuntimeError('TDC Core signature mismatch! Check that core '
                               'is mapped correctly')
        # Two revision checks are needed:
        #   FPGA Current Rev >= Host Oldest Compatible Rev
        #   Host Current Rev >= FPGA Oldest Compatible Rev
        fpga_current_revision = self.peek32(self.SYNC_REVISION) & 0xFFFFFFFF
        fpga_old_compat_revision = self.peek32(self.SYNC_OLDESTCOMPAT) & 0xFFFFFFFF
        if fpga_current_revision < self.oldest_compat_version:
            self.log.error("Revision check failed! MPM oldest supported revision "
                           "(0x{:08X}) is too new for this FPGA revision (0x{:08X})."
                           .format(self.oldest_compat_version, fpga_current_revision))
            raise RuntimeError('This MPM version does not support the loaded FPGA image.'
                               ' Please update images!')
        if self.current_version < fpga_old_compat_revision:
            self.log.error("Revision check failed! FPGA oldest compatible revision "
                           "(0x{:08X}) is too new for this MPM version (0x{:08X})."
                           .format(fpga_current_revision, self.current_version))
            raise RuntimeError('The loaded FPGA version is too new for MPM. '
                               'Please update MPM!')
        self.log.trace("TDC Core current revision: 0x{:08X}" \
                       .format(fpga_current_revision))
        self.log.trace("TDC Core oldest compatible revision: 0x{:08X}" \
                       .format(fpga_old_compat_revision))
        # Versioning notes:
        # TDC 1.0 = [0,          0x18021614)
        # TDC 2.0 = [0x18021614, today]
        if fpga_current_revision >= 0x18021614:
            self.tdc_rev = 2
        return True


    def master_reset(self):
        """
        Toggles the master reset for the registers as well as all other portions of
        the TDC. Confirms registers are cleared by writing and reading from the
        scratch register. Typically there is no need for this master reset to be
        toggled, but is presented here as a back-door.
        """
        # Write the scratch register with known data. This will be used to tell if the
        # register data is cleared.
        self.poke32(self.SYNC_SCRATCH, 0xDEADBEEF)
        assert self.peek32(self.SYNC_SCRATCH) == 0xDEADBEEF

        # Reset the register space first, then check to confirm it has been cleared.
        self.poke32(self.TDC_MASTER_RESET, 0x01) # self-clearing bit
        if self.peek32(self.SYNC_SCRATCH) != 0:
            self.log.error("TDC Failed to Clear Registers!")
            raise RuntimeError("TDC Master Reset Failed")

        # Toggle the master reset for the TDC module. This is not self-clearing.
        self.poke32(self.TDC_MASTER_RESET, 0x10)
        time.sleep(0.001)
        self.poke32(self.TDC_MASTER_RESET, 0x00)
        self.configured = False


    def run(self, num_meas, target_offset=0.0e-9):
        """
        Perform a basic synchronization routine by calling configure(), measure(), and
        align(). The last two calls are repeated for the length of num_meas, and the last
        call only reports the offset value without shifting the clocks.
        """

        self.log.debug("Starting clock synchronization...")
        # Configure the TDC once, then run as many measurements as desired. Force
        # configuration since we have no way of determining if the clock rates changed
        # since the last time it was configured.
        self.configure(force=True)
        # First measurement run to determine how far we need to adjust.
        for x in range(len(num_meas)):
            # On the last alignment run, only report the final offset value. If there is
            # only one run requested, then run the full alignment sequence.
            report_only = (len(num_meas) > 1) & (x == (len(num_meas)-1))
            meas   = self.measure(num_meas[x])
            offset = self.align(
                target_offset=target_offset,
                current_value=meas,
                report_only=report_only)
        return offset


    def configure(self, force=False):
        """
        Perform a soft reset on the TDC, then configure the TDC registers in the FPGA
        based on the reference and master clock rates. Enable the TDC and wait for the
        next PPS to arrive. Will throw on error. Otherwise returns nothing.
        """
        if self.configured:
            if not force:
                self.log.debug("TDC is already configured. " \
                               "Skipping configuration sequence!")
                return None
            else:
                # Apparently force is specified... this could lead to some strange
                # TDC behavior, but we do it anyway.
                self.log.debug("TDC is already configured, but Force is specified..." \
                               "reconfiguring the TDC anyway!")

        self.log.debug("Configuring the TDC...")
        self.log.trace("Using reference clock frequency: {:.3f} MHz" \
            .format(self.ref_clk_freq/1e6))
        self.log.trace("Using master clock frequency: {:.3f} MHz" \
            .format(self.radio_clk_freq/1e6))

        meas_clk_ref_freq = 166.666666666667e6
        if self.tdc_rev == 1:
            self.meas_clk_freq = meas_clk_ref_freq*5.5/1/5.375
        else:
            self.meas_clk_freq = meas_clk_ref_freq*21.875/3/6.125
        self.log.trace("Using measurement clock frequency: {:.10f} MHz" \
            .format(self.meas_clk_freq/1e6))

        self.configured = False
        # Reset and disable TDC, clear PPS crossing, and enable re-runs. Confirm the
        # core is in reset and PPS is cleared.
        self.poke32(self.TDC_CONTROL, 0x2121)
        reset_status = self.peek32(self.TDC_STATUS) & 0xFF
        if reset_status != 0x01:
            self.log.error("TDC Failed to Reset! Check your clocks! Status: 0x{:x}" \
                .format(reset_status))
            raise RuntimeError("TDC Failed to reset.")

        def find_rate(clk_freq, rates):
            """
            Go through the rates list in sequential order, checking each rate for
            even division into our clk_freq. Return the first one we find.
            """
            for rate in rates:
                if math.modf(clk_freq/rate)[0] == 0:
                    return rate
            self.log.error("TDC Setup Failure: Pulse rate setup failed for {:.4f} MHz!" \
                .format(clk_freq/1e6))
            raise RuntimeError("TDC Failed to Initialize. No pulse rate found!")

        def get_pulse_setup(clk_freq, pulser, compat_mode=False):
            """
            Set the pulser divide values based on the given clock rates.
            Returns register value required to create the desired pulses.
            """
            # Compatibility mode runs at 40 kHz. This only supports these clock rates:
            # 10, 20, 25, 125, 122.88, and 153.6 MHz. Any other rates are expected
            # to use the TDC 2.0 and later.
            if compat_mode:
                pulse_rate = find_rate(clk_freq, [40e3])
            elif (pulser == "rp") or (pulser == "sp"):
                pulse_rate = find_rate(clk_freq, self.SUPPORTED_PULSE_RATES)
            # The RP-t and SP-t pulsers always run at 10 kHz, which is the GCD of all
            # supported clock rates.
            elif (pulser == "rpt") or (pulser == "spt"):
                pulse_rate = find_rate(clk_freq, [10e3])
            else:
                self.log.error("TDC Setup Failure: Unrecognized pulse name given: {}" \
                    .format(pulser))
                raise RuntimeError("TDC Failed to Initialize. "
                                   "Unrecognized pulse name given!")

            period = int(clk_freq/pulse_rate)
            hi_time = int(math.floor(period/2))
            # All registers are packed [30:16] = high time, [15:0] = period.
            # hi_time is period/2 so we only use 15 bits.
            assert hi_time <= 0x7FFF and period <= 0xFFFF
            ctrl_word = (hi_time << 16) | period
            return pulse_rate, ctrl_word

        def get_restart_pulse_setup(rp_rate, sp_rate):
            """
            Set the restart pulser divide values based on the repeating pulse rates.
            Returns register value required to create the desired pulses.
            """
            # The Restart-pulser must run at the GCD of the RP and SP rates, not the
            # Reference Clock and Radio Clock rates!
            pulse_rate = find_rate(self.ref_clk_freq, [gcd(rp_rate, sp_rate)])
            period = int(self.ref_clk_freq/pulse_rate)
            hi_time = int(math.floor(period/2))
            # The re-pulse is broken into two registers:
            # -1 is the period and -2 is the high time
            assert period <= 0xFFFFFF # 24 bits
            assert hi_time <= 0x7FFFFF # 23 bits
            return pulse_rate, (period & 0xFFFFFF), (hi_time & 0x7FFFFF)

        compat_mode = self.tdc_rev < 2
        if compat_mode:
            self.log.warning("Running TDC in Compatibility Mode for v1.0!")

        rp_rate, rp_ctrl_word  = get_pulse_setup(self.ref_clk_freq,  "rp", compat_mode)
        sp_rate, sp_ctrl_word  = get_pulse_setup(self.radio_clk_freq,"sp", compat_mode)
        rpt_rate,rpt_ctrl_word = get_pulse_setup(self.ref_clk_freq,  "rpt")
        spt_rate,spt_ctrl_word = get_pulse_setup(self.radio_clk_freq,"spt")
        rep_rate, repulse_ctrl_word1, repulse_ctrl_word2 = \
            get_restart_pulse_setup(rp_rate, sp_rate)
        self.log.trace("Using RP  pulse rate: {} MHz".format(rp_rate/1e6))
        self.log.trace("Using SP  pulse rate: {} MHz".format(sp_rate/1e6))
        self.log.trace("Using RPT pulse rate: {} MHz".format(rpt_rate/1e6))
        self.log.trace("Using SPT pulse rate: {} MHz".format(spt_rate/1e6))
        self.log.trace("Using Restart pulse rate: {} MHz".format(rep_rate/1e6))
        self.log.trace("Setting RP  control word to: 0x{:08X}".format(rp_ctrl_word))
        self.log.trace("Setting SP  control word to: 0x{:08X}".format(sp_ctrl_word))
        self.log.trace("Setting RPT control word to: 0x{:08X}".format(rpt_ctrl_word))
        self.log.trace("Setting SPT control word to: 0x{:08X}".format(spt_ctrl_word))
        self.log.trace("Setting RePulse-1 control word to: 0x{:08X}" \
                       .format(repulse_ctrl_word1))
        self.log.trace("Setting RePulse-2 control word to: 0x{:08X}" \
                       .format(repulse_ctrl_word2))
        self.poke32(self.REPULSE_PERIOD_CONTROL_1, repulse_ctrl_word1)
        self.poke32(self.REPULSE_PERIOD_CONTROL_2, repulse_ctrl_word2)
        self.poke32(self.RP_PERIOD_CONTROL,  rp_ctrl_word)
        self.poke32(self.SP_PERIOD_CONTROL,  sp_ctrl_word)
        self.poke32(self.RPT_PERIOD_CONTROL, rpt_ctrl_word)
        self.poke32(self.SPT_PERIOD_CONTROL, spt_ctrl_word)

        # Take the core out of reset, then check the reset done bit cleared.
        self.poke32(self.TDC_CONTROL, 0x2)
        reset_status = self.peek32(self.TDC_STATUS) & 0xFF
        if reset_status != 0x00:
            self.log.error(
                "TDC Reset Failed to Clear! " \
                "Check that your clocks are toggling. Status: 0x{:x}"
                .format(reset_status)
            )
            raise RuntimeError("TDC Reset Failed.")

        # Set the PPS crossing delay from the SP-t rising edge to the PPS pulse
        # in the Radio Clock domain.
        # delay = [19..16], update = 20
        reg_val = (self.pps_out_pipe_var_delay & 0xF) << 16 | 0b1 << 20
        self.poke32(self.TDC_CONTROL, reg_val)

        # Set the pulser enable delay from the RePulse enable to the RP enable.
        # Actual delay is +1 whatever is written here. Default is 1.
        # delay = [27..24], update = 28
        reg_val = (self.pps_in_pipe_dynamic_delay & 0xF) << 24 | 0b1 << 28
        self.poke32(self.TDC_CONTROL, reg_val)

        # Enable the TDC to capture the PPS. As long as PPS is actually a PPS, this
        # doesn't have to happen "synchronously" across all devices. Each device can
        # choose a different PPS and still be aligned.
        self.log.trace("Enabling the TDC...")
        self.poke32(self.TDC_CONTROL, 0x10)

        # Since a PPS rising edge comes once per second, we only need to wait
        # slightly longer than a second (worst-case) to confirm the TDC
        # received a PPS.
        if not poll_with_timeout(
                lambda: bool(self.peek32(self.TDC_STATUS) == 0x10),
                1100, # Try for 1.1 seconds
                100, # Poll every 100 ms
            ):
            # Try one last time, just in case there is weirdness with the polling loop.
            if self.peek32(self.TDC_STATUS) != 0x10:
                self.log.error("Failed to capture PPS within 1.1 seconds. " \
                               "TDC_STATUS: 0x{:X}".format(self.peek32(self.TDC_STATUS)))
                raise RuntimeError("Failed to capture PPS.")
        self.log.trace("PPS Captured!")
        self.configured = True


    def measure(self, num_meas=512):
        """
        Read num_meas measurements from the device. Average them and return the final
        offset value.
        """

        # Make sure the TDC is configured before attempting to read measurements.
        if not self.configured:
            self.log.error("TDC is not configured prior to requesting measurements!")
            raise RuntimeError("TDC is not configured prior to requesting measurements!")

        measure_offset = lambda: self._read_tdc_meas(
            self.meas_clk_freq, self.ref_clk_freq, self.radio_clk_freq
        )

        # Retrieve the measurements.
        tdc_start_time = time.time()
        self.log.trace("Reading {} TDC measurements from device...".format(num_meas))
        measurements = [measure_offset() for _ in range(num_meas)]

        # All the measurements taken in a single run should be nearly identical. The
        # expected max delta between all measurements (from accuracy calculations)
        # is 1 ns. Take the average of the measurements and then compare each value mean
        # to see if it fits this criteria.
        current_value = mean(measurements)

        max_skew = 0.5e-9 # 500 ps of tolerated skew either direction
        meas_err = bool(sum([x < current_value-max_skew for x in measurements])) or \
                   bool(sum([x > current_value+max_skew for x in measurements]))
        meas_range = max(measurements) - min(measurements)

        self.log.trace("TDC Measurements Collected! Average = {:.3f} ns. "
                       "Range: {:.3f} ns".format(current_value*1e9, meas_range*1e9))
        self.log.trace("TDC Measurement Duration: {:.3f} s" \
                       .format(time.time()-tdc_start_time))
        if meas_err:
            self.log.error("TDC measurements show a wide range of values! "
                           "Check your clock rates for incompatibilities.")
            raise RuntimeError("TDC measurement out of expected range!")

        return current_value


    def align(self, target_offset=0.0e-12, current_value=0.0e-9, report_only=False):
        """
        Takes the current value and aligns the clock to the target. Optionally returns
        before performing any shifting if report_only is set to True.
        """

        # Make sure the TDC is configured before attempting to align.
        if not self.configured:
            self.log.error("TDC is not configured prior to requesting alignment!")
            raise RuntimeError("TDC is not configured prior to requesting alignment!")

        # The TDC 1.0 only supports homogeneous rate synchronization due to the re-run
        # architecture requiring the SP to occur after the RP. Set the target value to
        # any reasonable value that still accomplishes this purpose.
        if self.tdc_rev < 2:
            self.target_values = [1.0/self.ref_clk_freq + 3.5/self.radio_clk_freq]
            self.target_values = [x + target_offset for x in self.target_values]
        else:
            # Heterogeneous rate synchronization is only valid when using the same
            # reference clock source and period value. Compensate for the PPS output
            # pipeline delay by removing the integer number of Radio Clock cycles from
            # the target value.
            self.target_values = [0.0]
            pps_xing_delay = self.pps_out_pipe_var_delay + self.PPS_OUT_PIPE_STATIC_DELAY
            self.target_values = [x - pps_xing_delay/self.radio_clk_freq + target_offset \
                                for x in self.target_values]

        # Run the current value through the oracle to determine the adjustments to make.
        coarse_steps_required, dac_word_delta, distance_to_target = self._oracle(
            self.target_values,
            current_value,
            self.lmk_vco_freq,
            self.fine_delay_step
        )

        if not report_only:
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
            # crossing from this point forward, even if we re-run this routine,
            # until we reconfigure the core again with configure().
            self.poke32(self.TDC_CONTROL, 0x1000)

        return distance_to_target


    def _read_tdc_meas(
            self,
            meas_clk_freq=170.542641116e6,
            ref_clk_freq=10e6,
            radio_clk_freq=125e6,
        ):
        """
        Return the offset (in seconds) from the SP to the RP.
        """
        # Current worst-case time given a 40kHz pulse rate and 2^17 measurements for
        # the period average operation is ~3.28 s... Round up to 5.0 s. This value is
        # only for the first measurement to appear... subsequent repeat runs should be
        # only a few us long.
        timeout = time.time() + 5.0
        while True:
            sp_offset_msb = self.peek32(self.SP_OFFSET_1)
            if sp_offset_msb & 0x100 == 0x100:
                break
            if time.time() > timeout:
                error_msg = "Offsets failed to update within timeout."
                self.log.error(error_msg)
                raise RuntimeError(error_msg)

        # CRITICAL: These register values are locked when SP_OFFSET_1 is read and
        # reloaded when SP_OFFSET_1 is read again, to keep one value from updating before
        # the other. The SP and RP measurements are only meaningful when compared to one
        # another from the same TDC run.
        sp_offset_lsb = self.peek32(self.SP_OFFSET_0)
        rp_offset_msb = self.peek32(self.RP_OFFSET_1)
        rp_offset_lsb = self.peek32(self.RP_OFFSET_0)

        sp_offset = (sp_offset_msb & 0xFF) << 32
        sp_offset = (sp_offset | sp_offset_lsb)
        rp_offset = (rp_offset_msb & 0xFF) << 32
        rp_offset = (rp_offset | rp_offset_lsb)

        # Do the subtraction before converting to floating point.
        sp_rp = float(sp_offset - rp_offset) / (1<<27)

        # Some Math...
        # Convert the reading from meas_clk ticks to picoseconds
        sp_rp_samp = sp_rp/meas_clk_freq
        # True difference between the SP and RP pulses, due to sampling locations
        offset = sp_rp_samp + 1.0/ref_clk_freq - 1.0/radio_clk_freq
        return offset


    def _oracle(self, target_values, current_value, lmk_vco_freq, fine_delay_step):
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
        # the correct value. To combat this blatant crime, I just give it a positive value
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
            self.log.debug("Test Progress: {:.2f}%".format(x*100/(middle_samples+2)))
            self.write_dac_word(x*inc + low_bound, 0.1)
            distance_to_target = self.run_sync(measurement_only=True)
            meas_file.write("{}, {:.4f}\n".format(x*inc + low_bound, distance_to_target*1e12))
            results.append(distance_to_target*1e12)

        meas_file.close()

        self.log.info("Results:")
        for y in range(middle_samples + 2):
            self.log.info("Word: {}   Measurement: {:.6f}".format(y*inc + low_bound, results[y]))

        return True


    def dac_bist(self, taps_from_center=100):
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

