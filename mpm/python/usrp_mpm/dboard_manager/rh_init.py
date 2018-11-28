#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Helper class to initialize a Rhodium daughterboard
"""

from __future__ import print_function
import time
from usrp_mpm.sys_utils.uio import open_uio
from usrp_mpm.dboard_manager.lmk_rh import LMK04828Rh
from usrp_mpm.dboard_manager.rh_periphs import DboardClockControl
from usrp_mpm.cores import ClockSynchronizer
from usrp_mpm.cores import nijesdcore
from usrp_mpm.cores.eyescan import EyeScanTool
from usrp_mpm.dboard_manager.gain_rh import GainTableRh


class RhodiumInitManager(object):
    """
    Helper class: Holds all the logic to initialize an N320/N321 (Rhodium)
    daughterboard.
    """
    # The Phase DAC is set at midscale, having its flatness validate +/- 1023 codes
    # from this initial value.
    INIT_PHASE_DAC_WORD = 32768
    PHASE_DAC_SPI_ADDR  = 0x3
    # External PPS pipeline delay from the PPS captured at the FPGA to TDC input,
    # in reference clock ticks
    EXT_PPS_DELAY = 5
    # Variable PPS delay before the RP/SP pulsers begin. Fixed value for the N3xx devices.
    N3XX_INT_PPS_DELAY = 4
    # JESD core default configuration.
    JESD_DEFAULT_ARGS = {"lmfc_divider"   : 12,
                         "rx_sysref_delay": 5,
                         "tx_sysref_delay": 11,
                         "tx_driver_swing": 0b1101,
                         "tx_precursor"   : 0b00100,
                         "tx_postcursor"  : 0b00100}
    # After testing the roundtrip latency (i.e. FPGA -> TX -> RX -> FPGA),
    # it was found that a different value of RX SYSREF delay is required
    # for sampling_clock_rate = 400 MSPS to achieve latency consistency.
    RX_SYSREF_DLY_DIC = {400e6: 6, 491.52e6: 5, 500e6: 5}


    def __init__(self, rh_class, spi_ifaces):
        self.rh_class = rh_class
        self._spi_ifaces = spi_ifaces
        self.adc = rh_class.adc
        self.dac = rh_class.dac
        self.slot_idx = rh_class.slot_idx
        self.log = rh_class.log.getChild('init')


    def _init_lmk(self, lmk_spi, ref_clk_freq, sampling_clock_rate,
                  pdac_spi, init_phase_dac_word, phase_dac_spi_addr):
        """
        Sets the phase DAC to initial value, and then brings up the LMK
        according to the selected ref clock frequency.
        Will throw if something fails.
        """
        self.log.trace("Initializing Phase DAC to d{}.".format(
            init_phase_dac_word
        ))
        pdac_spi.poke16(phase_dac_spi_addr, init_phase_dac_word)
        return LMK04828Rh(self.slot_idx, lmk_spi, ref_clk_freq, sampling_clock_rate, self.log)


    def _sync_db_clock(self, dboard_ctrl_regs, ref_clk_freq, master_clock_rate, args):
        " Synchronizes the DB clock to the common reference "
        reg_offset = 0x200
        ext_pps_delay = self.EXT_PPS_DELAY
        if args.get('time_source', self.rh_class.default_time_source) == 'sfp0':
            reg_offset = 0x400
            ref_clk_freq = 62.5e6
            ext_pps_delay = 1 # only 1 flop between the WR core output and the TDC input
        synchronizer = ClockSynchronizer(
            dboard_ctrl_regs,
            self.rh_class.lmk,
            self._spi_ifaces['phase_dac'],
            reg_offset,
            master_clock_rate,
            ref_clk_freq,
            1.116E-12, # fine phase shift. TODO don't hardcode. This should live in the EEPROM
            self.INIT_PHASE_DAC_WORD,
            self.PHASE_DAC_SPI_ADDR,
            ext_pps_delay,
            self.N3XX_INT_PPS_DELAY,
            self.slot_idx)
        # The radio clock traces on the motherboard are 69 ps longer for Daughterboard B
        # than Daughterboard A. We want both of these clocks to align at the converters
        # on each board, so adjust the target value for DB B. This is an N3xx series
        # peculiarity and will not apply to other motherboards.
        trace_delay_offset = {0:  0.0e-0,
                              1: 69.0e-12}[self.slot_idx]
        offset_error = abs(synchronizer.run(
            num_meas=[512, 128],
            target_offset=trace_delay_offset))
        if offset_error > 100e-12:
            self.log.error("Residual clock synchronization offset error is invalid! "
                "Expected: <100 ps Actual: {:.1f} ps!".format(offset_error*1e12))
            raise RuntimeError("Clock synchronization offset error is greater than expected.")
        else:
            self.log.debug("Residual synchronization error: {:.1f} ps.".format(
                offset_error*1e12
            ))
        synchronizer = None
        self.log.debug("Sample Clock Synchronization Complete!")


    def set_jesd_rate(self, jesdcore, new_rate, current_jesd_rate, force=False):
        """
        Make the QPLL and GTX changes required to change the JESD204B core rate.
        """
        # The core is directly compiled for 500 MHz sample rate, which
        # corresponds to a lane rate of 5.0 Gbps. The same QPLL and GTX settings apply
        # for the 491.52 MHz sample rate.
        #
        # The lower non-LTE rate, 400 MHz, requires changes to the default configuration
        # of the MGT components. This function performs the required changes in the
        # following order (as recommended by UG476).
        #
        # 1) Modify any QPLL settings.
        # 2) Perform the QPLL reset routine by pulsing reset then waiting for lock.
        # 3) Modify any GTX settings.
        # 4) Perform the GTX reset routine by pulsing reset and waiting for reset done.

        assert new_rate in (4000e6, 4915.2e6, 5000e6)

        # On first run, we have no idea how the FPGA is configured... so let's force an
        # update to our rate.
        force = force or (current_jesd_rate is None)

        skip_drp = False
        if not force:
            #           Current     New       Skip?
            skip_drp = {4000.0e6 : {4000.0e6: True , 4915.2e6: False, 5000.0e6: False},
                        4915.2e6 : {4000.0e6: False, 4915.2e6: True , 5000.0e6: True },
                        5000.0e6 : {4000.0e6: False, 4915.2e6: True , 5000.0e6: True }}[current_jesd_rate][new_rate]

        if skip_drp:
            self.log.trace("Current lane rate is compatible with the new rate. Skipping "
                           "reconfiguration.")

        # These are the only registers in the QPLL and GTX that change based on the
        # selected line rate. The MGT wizard IP was generated for each of the rates and
        # reference clock frequencies and then diffed to create this table.
        QPLL_CFG         = {4000.0e6: 0x6801C1, 4915.2e6: 0x680181, 5000.0e6: 0x0680181}[new_rate]
        MGT_RX_CLK25_DIV = {4000.0e6:        8, 4915.2e6:       10, 5000.0e6:        10}[new_rate]
        MGT_TX_CLK25_DIV = {4000.0e6:        8, 4915.2e6:       10, 5000.0e6:        10}[new_rate]

        # 1-2) Do the QPLL first
        if not skip_drp:
            self.log.trace("Changing QPLL settings to support {} Gbps".format(new_rate/1e9))
            jesdcore.set_drp_target('qpll', 0)
            # QPLL_CONFIG is spread across two regs: 0x32 (dedicated) and 0x33 (shared)
            reg_x32 = QPLL_CFG & 0xFFFF # [16:0] -> [16:0]
            reg_x33 = jesdcore.drp_access(rd=True, addr=0x33)
            reg_x33 = (reg_x33 & 0xF800) | ((QPLL_CFG >> 16) & 0x7FF)  # [26:16] -> [11:0]
            jesdcore.drp_access(rd=False, addr=0x32, wr_data=reg_x32)
            jesdcore.drp_access(rd=False, addr=0x33, wr_data=reg_x33)

        # Run the QPLL reset sequence and prep the MGTs for modification.
        jesdcore.init()

        # 3-4) And the 4 MGTs second
        if not skip_drp:
            self.log.trace("Changing MGT settings to support {} Gbps"
                           .format(new_rate/1e9))
            for lane in range(4):
                jesdcore.set_drp_target('mgt', lane)
                # MGT_RX_CLK25_DIV is embedded with others in 0x11. The
                # encoding for the DRP register value is one less than the
                # desired value.
                reg_x11 = jesdcore.drp_access(rd=True, addr=0x11)
                reg_x11 = (reg_x11 & 0xF83F) | \
                          ((MGT_RX_CLK25_DIV-1 & 0x1F) << 6) # [10:6]
                jesdcore.drp_access(rd=False, addr=0x11, wr_data=reg_x11)
                # MGT_TX_CLK25_DIV is embedded with others in 0x6A. The
                # encoding for the DRP register value is one less than the
                # desired value.
                reg_x6a = jesdcore.drp_access(rd=True, addr=0x6A)
                reg_x6a = (reg_x6a & 0xFFE0) | (MGT_TX_CLK25_DIV-1 & 0x1F) # [4:0]
                jesdcore.drp_access(rd=False, addr=0x6A, wr_data=reg_x6a)
            self.log.trace("GTX settings changed to support {} Gbps"
                           .format(new_rate/1e9))
            jesdcore.disable_drp_target()

        self.log.trace("JESD204b Lane Rate set to {} Gbps!"
                       .format(new_rate/1e9))
        return new_rate


    def init_jesd(self, jesdcore, sampling_clock_rate):
        """
        Bringup the JESD links between the ADC, DAC, and the FPGA.
        All clocks must be set up and stable before starting this routine.
        """
        jesdcore.check_core()

        # JESD Lane Rate only depends on the sampling_clock_rate selection, since all
        # other link parameters (LMFS,N) remain constant.
        L = 4
        M = 2
        F = 1
        S = 1
        N = 16
        new_rate = sampling_clock_rate * M * N * (10.0/8) / L / S
        self.log.trace("Calculated JESD204B lane rate is {} Gbps".format(new_rate/1e9))
        self.rh_class.current_jesd_rate = \
            self.set_jesd_rate(jesdcore, new_rate, self.rh_class.current_jesd_rate)

        self.log.trace("Setting up JESD204B TX blocks.")
        jesdcore.init_framer()                # Initialize FPGA's framer.
        self.adc.init_framer()                # Initialize ADC's framer.

        self.log.trace("Enabling SYSREF capture blocks.")
        self.dac.enable_sysref_capture(True)  # Enable DAC's SYSREF capture.
        self.adc.enable_sysref_capture(True)  # Enable ADC's SYSREF capture.
        jesdcore.enable_lmfc(True)            # Enable FPGA's SYSREF capture.

        self.log.trace("Setting up JESD204B DAC RX block.")
        self.dac.init_deframer()              # Initialize DAC's deframer.

        self.log.trace("Sending SYSREF to all devices.")
        jesdcore.send_sysref_pulse()          # Send SYSREF to all devices.

        self.log.trace("Setting up JESD204B FPGA RX block.")
        jesdcore.init_deframer()              # Initialize FPGA's deframer.

        self.log.trace("Disabling SYSREF capture blocks.")
        self.dac.enable_sysref_capture(False) # Disable DAC's SYSREF capture.
        self.adc.enable_sysref_capture(False) # Disable ADC's SYSREF capture.
        jesdcore.enable_lmfc(False)           # Disable FPGA's SYSREF capture.

        time.sleep(0.100)                     # Allow time for CGS/ILA.

        self.log.trace("Verifying JESD204B link status.")
        error_flag = False
        if not jesdcore.get_framer_status():
            self.log.error("JESD204b FPGA Core Framer is not synced!")
            error_flag = True
        if not self.dac.check_deframer_status():
            self.log.error("DAC JESD204B Deframer is not synced!")
            error_flag = True
        if not self.adc.check_framer_status():
            self.log.error("ADC JESD204B Framer is not synced!")
            error_flag = True
        if not jesdcore.get_deframer_status():
            self.log.error("JESD204B FPGA Core Deframer is not synced!")
            error_flag = True
        if error_flag:
            raise RuntimeError('JESD204B Link Initialization Failed. See MPM logs for details.')
        self.log.info("JESD204B Link Initialization & Training Complete")


    def init(self, args):
        """
        Run the full initialization sequence. This will bring everything up
        from scratch: The LMK, JESD cores, the AD9695, the DAC37J82, and
        anything else that is clocking-related.
        Depending on the settings, this can take a fair amount of time.
        """
        # Input validation on RX margin tests (@ FPGA and DAC)
        # By accepting the rx_eyescan/tx_prbs argument being str or bool, one may
        # request an eyescan measurement to be performed from either the USRP's
        # shell (i.e. using --default-args) or from the host's MPM shell.
        perform_rx_eyescan = False
        if 'rx_eyescan' in args:
            perform_rx_eyescan = (args['rx_eyescan'] == 'True') or (args['rx_eyescan'] == True)
        if perform_rx_eyescan:
            self.log.trace("Adding RX eye scan PMA enable to JESD args.")
            self.JESD_DEFAULT_ARGS["enable_rx_eyescan"] = True
        perform_tx_prbs = False
        if 'tx_prbs' in args:
            perform_tx_prbs = (args['tx_prbs'] == 'True') or (args['tx_prbs'] == True)

        # Latency across the JESD204B TX/RX links should remain constant and
        # deterministic across the supported sampling_clock_rate values.
        # After testing the roundtrip latency (i.e. FPGA -> TX -> RX -> FPGA),
        # it was found that a different set of SYSREF delay values are required
        # for sampling_clock_rate = 400 MSPS to achieve latency consistency.
        self.JESD_DEFAULT_ARGS['rx_sysref_delay'] = \
          self.RX_SYSREF_DLY_DIC[self.rh_class.sampling_clock_rate]

        # Bringup Sequence.
        #   1. Prerequisites (include opening mmaps)
        #   2. Initialize LMK and bringup clocks.
        #   3. Synchronize DB Clocks.
        #   4. Initialize FPGA JESD IP.
        #   5. DAC Configuration.
        #   6. ADC Configuration.
        #   7. JESD204B Initialization.
        #   8. CPLD Gain Tables Initialization.

        # 1. Prerequisites
        # Open FPGA IP (Clock control and JESD core).
        self.log.trace("Creating gain table object...")
        self.gain_table_loader = GainTableRh(
            self._spi_ifaces['cpld'],
            self._spi_ifaces['cpld_gain_loader'],
            self.log)

        with open_uio(
            label="dboard-regs-{}".format(self.rh_class.slot_idx),
            read_only=False
        ) as radio_regs:
            self.log.trace("Creating dboard clock control object")
            db_clk_control = DboardClockControl(radio_regs, self.log)
            self.log.trace("Creating jesdcore object")
            jesdcore = nijesdcore.NIJESDCore(radio_regs,
                                             self.rh_class.slot_idx,
                                             **self.JESD_DEFAULT_ARGS)

            # 2. Initialize LMK and bringup clocks.
            # Disable FPGA MMCM's outputs, and assert its reset.
            db_clk_control.reset_mmcm()
            # Always place the JESD204b cores in reset before modifying the clocks,
            # otherwise high power or erroneous conditions could exist in the FPGA!
            jesdcore.reset()
            # Configure and bringup the LMK's clocks.
            self.log.trace("Initializing LMK...")
            self.rh_class.lmk = self._init_lmk(
                self._spi_ifaces['lmk'],
                self.rh_class.ref_clock_freq,
                self.rh_class.sampling_clock_rate,
                self._spi_ifaces['phase_dac'],
                self.INIT_PHASE_DAC_WORD,
                self.PHASE_DAC_SPI_ADDR
            )
            self.log.trace("LMK Initialized!")
            # Deassert FPGA's MMCM reset, poll for lock, and enable outputs.
            db_clk_control.enable_mmcm()

            # 3. Synchronize DB Clocks.
            # The clock synchronzation driver receives the master_clock_rate, which for
            # Rhodium is half the sampling_clock_rate.
            self._sync_db_clock(
                radio_regs,
                self.rh_class.ref_clock_freq,
                self.rh_class.sampling_clock_rate / 2,
                args)

            # 4. DAC Configuration.
            self.dac.config()

            # 5. ADC Configuration.
            self.adc.config()

            # 6-7. JESD204B Initialization.
            self.init_jesd(jesdcore, self.rh_class.sampling_clock_rate)
            # [Optional] Perform RX eyescan.
            if perform_rx_eyescan:
                self.log.info("Performing RX eye scan on ADC to FPGA link...")
                self._rx_eyescan(jesdcore, args)
            # [Optional] Perform TX PRBS test.
            if perform_tx_prbs:
                self.log.info("Performing TX PRBS-31 test on FPGA to DAC link...")
                self._tx_prbs_test(jesdcore, args)
            # Direct the garbage collector by removing our references
            jesdcore = None
            db_clk_control = None

        # 8. CPLD Gain Tables Initialization.
        self.gain_table_loader.init()

        return True


    ##########################################################################
    # JESD204B RX margin testing
    ##########################################################################

    def _rx_eyescan(self, jesdcore, args):
        """
        This function creates an eyescan object to perform this measurement with the
        given configuration and lanes.

        Parameters:
          prescale   -> Controls the prescaling of the sample count to keep both sample
                        count and error count in reasonable precision.
                        Valid values: from 0 to 31.
        """
        # The following constants must be defined according to GTs configuration
        # for each project. For further details, refer to the eyescan.py file.
        # For Rhodium, these parameters are based on the JESD core.
        rxout_div        = 2
        rx_int_datawidth = 20
        eq_mode          = 'LPM'
        # The following variables define the GTs to be scanned and the range of the
        # measurement.
        prescale   = 0
        scan_lanes = [0, 1, 2, 3]
        hor_range  = {'start':-32 , 'stop':32 , 'step': 2}
        ver_range  = {'start':-127, 'stop':127, 'step': 2}
        # Set default configuration values for Rhodium when the user is not intentionally
        # changing the constants/variables default values.
        for key in ('rxout_div', 'rx_int_datawidth', 'eq_mode',
                    'prescale', 'scan_lanes', 'hor_range', 'ver_range'):
            if key not in args:
                self.log.trace("Setting Rh default value for {0}... val: {1}"
                               .format(key, locals()[key]))
                args[key] = locals()[key]
        #
        # Create an eyescan object.
        assert jesdcore is not None
        eyescan_tool = EyeScanTool(jesdcore, self.slot_idx, **args)
        # Put the ADC in pseudorandom test mode.
        adc_regs = self._spi_ifaces['adc']
        # test_val = adc_regs.peek8(0x0550)
        # adc_regs.poke8(0x0550, 0x05)
        test_val = adc_regs.peek8(0x0573)
        adc_regs.poke8(0x0573, 0x13)
        # Perform eye scan on given lanes and range.
        file_name = eyescan_tool.eyescan_full_scan(args['scan_lanes'],
                                                   args['hor_range'], args['ver_range'])
        # Do some housekeeping...
        # adc_regs.poke8(0x0550, test_val) # Enable normal operation.
        adc_regs.poke8(0x0573, test_val) # Enable normal operation.
        adc_regs.poke8(0x0000, 0x81) # Reset.
        eyescan_tool = None
        return file_name

    def _tx_prbs_test(self, jesdcore, args):
        """
        This function allows to test the PRBS-31 pattern at the DAC.
        """
        def _test_lanes(**tx_settings):
            """
            This methods enables, monitors, and disables the PRBS-31 test.
            """
            results = []
            jesdcore.adjust_tx_phy(**tx_phy_settings)
            self.log.info("Testing TX PHY settings: tx_driver_swing=0b{0:04b}"
                                                 "  tx_precursor=0b{1:05b}"
                                                 "  tx_postcursor=0b{2:05b}"
                          .format(tx_phy_settings["tx_driver_swing"],
                                  tx_phy_settings["tx_precursor"],
                                  tx_phy_settings["tx_postcursor"]))
            # Enable the GTs TX pattern generator in PRBS-31 mode.
            jesdcore.set_pattern_gen(mode='PRBS-31')
            # Monitor each receive lane at DAC.
            for lane_num in range(0, 4):
                self.dac.test_mode(mode='PRBS-31', lane=lane_num) # Enable PRBS test mode.
                number_of_failures = 0
                for _ in range(0, POLLS_PER_GT):
                    time.sleep(WAIT_TIME_PER_POLL)
                    alarm_pin_dac = self.rh_class.cpld.get_dac_alarm()
                    if alarm_pin_dac:
                        number_of_failures += 1
                results.append(number_of_failures)
                if number_of_failures > 0:
                    self.log.error("PRBS-31 test for DAC lane {0} failed {1}/{2}!"
                                   .format(lane_num, number_of_failures, POLLS_PER_GT))
                else:
                    self.log.info("PRBS-31 test for DAC lane {0} passed!"
                                  .format(lane_num))
                self.dac.test_mode(mode='OFF', lane=lane_num) # Disable PRBS test mode.
            # Disable TX pattern generator at FPGA
            jesdcore.set_pattern_gen(mode='OFF')
            return results
        #
        WAIT_TIME_PER_POLL = 0.001 # in seconds.
        POLLS_PER_GT = 100
        # Create the CSV file.
        f = open('tx_prbs_sweep.csv', 'w')
        f.write("Swing,Precursor,Postcursor,Polls,Failures 0,Failures 1,Failures 2,Failures 3\n")
        # Default TX PHY settings.
        tx_phy_settings = {"tx_driver_swing": 0b1111,  # See UG476, TXDIFFCTRL
                           "tx_precursor"   : 0b00000, # See UG476, TXPRECURSOR
                           "tx_postcursor"  : 0b00000} # See UG476, TXPOSTCURSOR
        # Define sweep ranges.
        DEFAULT_SWING_RANGE = {'start': 0b0000, 'stop': 0b1111 + 0b1, 'step': 1}
        DEFAULT_CURSOR_RANGE = {'start': 0b00000, 'stop': 0b11111 + 0b1, 'step': 2}
        swing_range = args.get("swing_range", DEFAULT_SWING_RANGE)
        precursor_range = args.get("precursor_range", DEFAULT_CURSOR_RANGE)
        postcursor_range = args.get("postcursor_range", DEFAULT_CURSOR_RANGE)
        # Test the TX margin across multiple PHY settings.
        for swing in range(swing_range['start'], swing_range['stop'], swing_range['step']):
            tx_phy_settings["tx_driver_swing"] = swing
            for precursor in range(precursor_range['start'], precursor_range['stop'], precursor_range['step']):
                tx_phy_settings["tx_precursor"] = precursor
                for postcursor in range(postcursor_range['start'], postcursor_range['stop'], postcursor_range['step']):
                    tx_phy_settings["tx_postcursor"] = postcursor
                    results = _test_lanes(**tx_phy_settings)
                    f.write("{},{},{},{},{},{},{},{}\n".format(
                            tx_phy_settings["tx_driver_swing"],
                            tx_phy_settings["tx_precursor"],
                            tx_phy_settings["tx_postcursor"],
                            POLLS_PER_GT, results[0], results[1], results[2], results[3]))
        # Housekeeping...
        f.close()
