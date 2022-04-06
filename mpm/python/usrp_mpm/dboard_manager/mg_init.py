#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Helper class to initialize a Magnesium daughterboard
"""

from __future__ import print_function
import re
import time
import math
from builtins import object

from usrp_mpm.sys_utils.uio import open_uio
from usrp_mpm.dboard_manager.lmk_mg import LMK04828Mg
from usrp_mpm.dboard_manager.mg_periphs import DboardClockControl
from usrp_mpm.cores import ClockSynchronizer
from usrp_mpm.cores import nijesdcore
from usrp_mpm.mpmutils import async_exec, str2bool

INIT_CALIBRATION_TABLE = {"TX_BB_FILTER"              :   0x0001,
                          "ADC_TUNER"                 :   0x0002,
                          "TIA_3DB_CORNER"            :   0x0004,
                          "DC_OFFSET"                 :   0x0008,
                          "TX_ATTENUATION_DELAY"      :   0x0010,
                          "RX_GAIN_DELAY"             :   0x0020,
                          "FLASH_CAL"                 :   0x0040,
                          "PATH_DELAY"                :   0x0080,
                          "TX_LO_LEAKAGE_INTERNAL"    :   0x0100,
                          "TX_LO_LEAKAGE_EXTERNAL"    :   0x0200,
                          "TX_QEC_INIT"               :   0x0400,
                          "LOOPBACK_RX_LO_DELAY"      :   0x0800,
                          "LOOPBACK_RX_RX_QEC_INIT"   :   0x1000,
                          "RX_LO_DELAY"               :   0x2000,
                          "RX_QEC_INIT"               :   0x4000,
                          "BASIC"                     :   0x4F,
                          "OFF"                       :   0x00,
                          "DEFAULT"                   :   0x4DFF,
                          "ALL"                       :   0x7DFF,
                         }

TRACKING_CALIBRATION_TABLE = {"TRACK_RX1_QEC"         :   0x01,
                              "TRACK_RX2_QEC"         :   0x02,
                              "TRACK_ORX1_QEC"        :   0x04,
                              "TRACK_ORX2_QEC"        :   0x08,
                              "TRACK_TX1_LOL"         :   0x10,
                              "TRACK_TX2_LOL"         :   0x20,
                              "TRACK_TX1_QEC"         :   0x40,
                              "TRACK_TX2_QEC"         :   0x80,
                              "OFF"                   :   0x00,
                              "RX_QEC"                :   0x03,
                              "TX_QEC"                :   0xC0,
                              "TX_LOL"                :   0x30,
                              "DEFAULT"               :   0xC3,
                              "ALL"                   :   0xF3,
                             }



class MagnesiumInitManager(object):
    """
    Helper class: Holds all the logic to initialize an N310/N300 (Magnesium)
    daughterboard.
    """
    # DAC is initialized to midscale automatically on power-on: 16-bit DAC, so
    # midpoint is at 2^15 = 32768. However, the linearity of the DAC is best
    # just below that point, so we set it to the (carefully calculated)
    # alternate value instead.
    INIT_PHASE_DAC_WORD = 31000 # Intentionally decimal
    PHASE_DAC_SPI_ADDR = 0x0
    # External PPS pipeline delay from the PPS captured at the FPGA to TDC input,
    # in reference clock ticks
    EXT_PPS_DELAY = 5
    # Variable PPS delay before the RP/SP pulsers begin. Fixed value for the
    # N3xx devices.
    N3XX_INT_PPS_DELAY = 4
    # JESD core default configuration.
    JESD_DEFAULT_ARGS = {"bypass_descrambler": False,
                         "lmfc_divider"      : 20,
                         "rx_sysref_delay"   : 8,
                         "tx_sysref_delay"   : 11}

    def __init__(self, mg_class, spi_ifaces):
        self.mg_class = mg_class
        self._spi_ifaces = spi_ifaces
        self.mykonos = mg_class.mykonos
        self.slot_idx = mg_class.slot_idx
        self.log = mg_class.log.getChild('init')

    def check_mykonos_framer_status(self):
        " Return True if Mykonos Framer is in good state "
        rb = self.mykonos.get_framer_status()
        self.log.trace("Mykonos Framer Status Register: 0x{:04X}".format(rb & 0xFF))
        tx_state =   {0: 'CGS',
                      1: 'ILAS',
                      2: 'ADC Data'}[rb & 0b11]
        ilas_state = {0: 'CGS',
                      1: '1st Multframe',
                      2: '2nd Multframe',
                      3: '3rd Multframe',
                      4: '4th Multframe',
                      5: 'Last Multframe',
                      6: 'invalid state',
                      7: 'ILAS Complete'}[(rb & 0b11100) >> 2]
        sysref_rx =              (rb & (0b1 << 5)) > 0
        fifo_ptr_delta_changed = (rb & (0b1 << 6)) > 0
        sysref_phase_error =     (rb & (0b1 << 7)) > 0
        # According to emails with ADI, fifo_ptr_delta_changed may be buggy.
        # Deterministic latency is still achieved even when this bit is toggled, so
        # ADI's recommendation is to ignore it. The expected state of this bit 0, but
        # occasionally it toggles to 1. It is unclear why exactly this happens.
        success = ((tx_state == 'ADC Data') &
                   (ilas_state == 'ILAS Complete') &
                   sysref_rx &
                   (not sysref_phase_error))
        logger = self.log.trace if success else self.log.warning
        logger("Mykonos Framer, TX State: %s", tx_state)
        logger("Mykonos Framer, ILAS State: %s", ilas_state)
        logger("Mykonos Framer, SYSREF Received: {}".format(sysref_rx))
        logger("Mykonos Framer, FIFO Ptr Delta Change: {} (ignored, possibly buggy)"
               .format(fifo_ptr_delta_changed))
        logger("Mykonos Framer, SYSREF Phase Error: {}"
               .format(sysref_phase_error))
        return success


    def check_mykonos_deframer_status(self):
        " Return True if Mykonos Deframer is in good state "
        rb = self.mykonos.get_deframer_status()
        self.log.trace("Mykonos Deframer Status Register: 0x{:04X}".format(rb & 0xFF))

        frame_symbol_error =  (rb & (0b1 << 0)) > 0
        ilas_multifrm_error = (rb & (0b1 << 1)) > 0
        ilas_framing_error =  (rb & (0b1 << 2)) > 0
        ilas_checksum_valid = (rb & (0b1 << 3)) > 0
        prbs_error =          (rb & (0b1 << 4)) > 0
        sysref_received =     (rb & (0b1 << 5)) > 0
        deframer_irq =        (rb & (0b1 << 6)) > 0
        success = ((not frame_symbol_error) &
                   (not ilas_multifrm_error) &
                   (not ilas_framing_error) &
                   ilas_checksum_valid &
                   (not prbs_error) &
                   sysref_received &
                   (not deframer_irq))
        logger = self.log.trace if success else self.log.warning
        logger("Mykonos Deframer, Frame Symbol Error: {}".format(frame_symbol_error))
        logger("Mykonos Deframer, ILAS Multiframe Error: {}".format(ilas_multifrm_error))
        logger("Mykonos Deframer, ILAS Frame Error: {}".format(ilas_framing_error))
        logger("Mykonos Deframer, ILAS Checksum Valid: {}".format(ilas_checksum_valid))
        logger("Mykonos Deframer, PRBS Error: {}".format(prbs_error))
        logger("Mykonos Deframer, SYSREF Received: {}".format(sysref_received))
        logger("Mykonos Deframer, Deframer IRQ Received: {}".format(deframer_irq))
        return success


    def _init_lmk(
            self,
            lmk_spi,
            ref_clock_freq,
            master_clock_rate,
            pdac_spi,
            init_phase_dac_word,
            phase_dac_spi_addr):
        """
        Sets the phase DAC to initial value, and then brings up the LMK
        according to the selected ref clock frequency.
        Will throw if something fails.
        """
        self.log.trace("Initializing Phase DAC to d{}.".format(
            init_phase_dac_word
        ))
        pdac_spi.poke16(phase_dac_spi_addr, init_phase_dac_word)
        return LMK04828Mg(
            lmk_spi,
            self.mg_class.spi_lock,
            ref_clock_freq,
            master_clock_rate,
            self.log
        )


    def _sync_db_clock(
            self,
            dboard_ctrl_regs,
            master_clock_rate,
            ref_clock_freq,
            args):
        """ Synchronizes the DB clock to the common reference """
        reg_offset = 0x200
        ext_pps_delay = self.EXT_PPS_DELAY
        if args.get('time_source', self.mg_class.default_time_source) == 'sfp0':
            reg_offset = 0x400
            ref_clock_freq = 62.5e6
            ext_pps_delay = 1 # only 1 flop between the WR core output and the TDC input
        synchronizer = ClockSynchronizer(
            dboard_ctrl_regs,
            self.mg_class.lmk,
            self._spi_ifaces['phase_dac'],
            reg_offset,
            master_clock_rate,
            ref_clock_freq,
            860E-15, # fine phase shift. TODO don't hardcode. This should live in the EEPROM
            self.INIT_PHASE_DAC_WORD,
            self.PHASE_DAC_SPI_ADDR,
            ext_pps_delay,
            self.N3XX_INT_PPS_DELAY,
            self.slot_idx
        )
        # The radio clock traces on the motherboard are 69 ps longer for
        # Daughterboard B than Daughterboard A. We want both of these clocks to
        # align at the converters on each board, so adjust the target value for
        # DB B. This is an N3xx series peculiarity and will not apply to other
        # motherboards.
        trace_delay_offset = {0:  0.0e-0,
                              1: 69.0e-12}[self.slot_idx]
        offset = synchronizer.run(
            num_meas=[512, 128],
            target_offset=trace_delay_offset)
        offset_error = abs(offset)
        if offset_error > 100e-12:
            self.log.error("Clock synchronizer measured an offset of {:.1f} ps!".format(
                offset_error*1e12
            ))
            raise RuntimeError("Clock synchronizer measured an offset of {:.1f} ps!".format(
                offset_error*1e12
            ))
        else:
            self.log.debug("Residual synchronization error: {:.1f} ps.".format(
                offset_error*1e12
            ))
        synchronizer = None # Help garbage collector
        self.log.debug("Sample Clock Synchronization Complete!")


    def set_jesd_rate(self, jesdcore, new_rate, current_jesd_rate, force=False):
        """
        Make the QPLL and GTX changes required to change the JESD204B core rate.
        """
        # The core is directly compiled for 125 MHz sample rate, which
        # corresponds to a lane rate of 2.5 Gbps. The same QPLL and GTX settings
        # apply for the 122.88 MHz sample rate.
        #
        # The higher LTE rate, 153.6 MHz, requires changes to the default
        # configuration of the MGT components. This function performs the
        # required changes in the following order (as recommended by UG476).
        #
        # 1) Modify any QPLL settings.
        # 2) Perform the QPLL reset routine by pulsing reset then waiting for lock.
        # 3) Modify any GTX settings.
        # 4) Perform the GTX reset routine by pulsing reset and waiting for reset done.
        assert new_rate in (2457.6e6, 2500e6, 3072e6)

        # On first run, we have no idea how the FPGA is configured... so let's force an
        # update to our rate.
        force = force or current_jesd_rate is None

        skip_drp = False
        if not force:
            #           Current     New       Skip?
            skip_drp = {2457.6e6 : {2457.6e6: True,  2500.0e6: True,  3072.0e6:False},
                        2500.0e6 : {2457.6e6: True,  2500.0e6: True,  3072.0e6:False},
                        3072.0e6 : {2457.6e6: False, 2500.0e6: False, 3072.0e6:True}}[self.mg_class.current_jesd_rate][new_rate]
        if skip_drp:
            self.log.trace(
                "Current lane rate is compatible with the new rate. "
                "Skipping reconfiguration.")

        # These are the only registers in the QPLL and GTX that change based on the
        # selected line rate. The MGT wizard IP was generated for each of the rates and
        # reference clock frequencies and then diffed to create this table.
        QPLL_CFG         = {2457.6e6: 0x680181, 2500e6: 0x680181, 3072e6: 0x06801C1}[new_rate]
        QPLL_FBDIV       = {2457.6e6:    0x120, 2500e6:    0x120, 3072e6:      0x80}[new_rate]
        MGT_PMA_RSV      = {2457.6e6: 0x1E7080, 2500e6: 0x1E7080, 3072e6:   0x18480}[new_rate]
        MGT_RX_CLK25_DIV = {2457.6e6:        5, 2500e6:        5, 3072e6:         7}[new_rate]
        MGT_TX_CLK25_DIV = {2457.6e6:        5, 2500e6:        5, 3072e6:         7}[new_rate]
        MGT_RXOUT_DIV    = {2457.6e6:        4, 2500e6:        4, 3072e6:         2}[new_rate]
        MGT_TXOUT_DIV    = {2457.6e6:        4, 2500e6:        4, 3072e6:         2}[new_rate]
        MGT_RXCDR_CFG    = {2457.6e6:0x03000023ff10100020, 2500e6:0x03000023ff10100020, 3072e6:0x03000023ff10200020}[new_rate]

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
            # QPLL_FBDIV is shared with other settings in reg 0x36
            reg_x36 = jesdcore.drp_access(rd=True, addr=0x36)
            reg_x36 = (reg_x36 & 0xFC00) | (QPLL_FBDIV & 0x3FF)  # in bits [9:0]
            jesdcore.drp_access(rd=False, addr=0x36, wr_data=reg_x36)

        # Run the QPLL reset sequence and prep the MGTs for modification.
        jesdcore.init()

        # 3-4) And the 4 MGTs second
        if not skip_drp:
            self.log.trace("Changing MGT settings to support {} Gbps"
                           .format(new_rate/1e9))
            for lane in range(4):
                jesdcore.set_drp_target('mgt', lane)
                # MGT_PMA_RSV is split over 0x99 (LSBs) and 0x9A
                reg_x99 = MGT_PMA_RSV & 0xFFFF
                reg_x9a = (MGT_PMA_RSV >> 16) & 0xFFFF
                jesdcore.drp_access(rd=False, addr=0x99, wr_data=reg_x99)
                jesdcore.drp_access(rd=False, addr=0x9A, wr_data=reg_x9a)
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
                # MGT_RXCDR_CFG is split over 0xA8 (LSBs) through 0xAD
                for reg_num, reg_addr in enumerate(range(0xA8, 0xAE)):
                    reg_data = (MGT_RXCDR_CFG >> 16*reg_num) & 0xFFFF
                    jesdcore.drp_access(rd=False, addr=reg_addr, wr_data=reg_data)
                # MGT_RXOUT_DIV and MGT_TXOUT_DIV are embedded together in
                # 0x88. The encoding for the DRP register value is
                # drp_val=log2(attribute)
                reg_x88 = (int(math.log(MGT_RXOUT_DIV, 2)) & 0x7) | \
                         ((int(math.log(MGT_TXOUT_DIV, 2)) & 0x7) << 4) # RX=[2:0] TX=[6:4]
                jesdcore.drp_access(rd=False, addr=0x88, wr_data=reg_x88)
            self.log.trace("GTX settings changed to support {} Gbps"
                           .format(new_rate/1e9))
            jesdcore.disable_drp_target()
        self.log.trace("JESD204b Lane Rate set to {} Gbps!"
                       .format(new_rate/1e9))
        self.mg_class.current_jesd_rate = new_rate
        return


    def init_lo_source(self, args):
        """Configure LO sources

        This function will initialize all LO sources to user specified sources.
        If there's no source is specified, the default one will be used.

        Arguments:
            args {string:string} -- device arguments.
        """
        self.log.debug("Setting up LO source..")
        rx_lo_source = args.get("rx_lo_source", "internal")
        tx_lo_source = args.get("tx_lo_source", "internal")
        self.mykonos.set_lo_source("RX", rx_lo_source)
        self.mykonos.set_lo_source("TX", tx_lo_source)
        self.log.debug("RX LO source is set at {}".format(self.mykonos.get_lo_source("RX")))
        self.log.debug("TX LO source is set at {}".format(self.mykonos.get_lo_source("TX")))


    def init_rf_cal(self, args):
        """ Setup RF CAL """
        def _parse_and_convert_cal_args(table, cal_args):
            """Parse calibration string and convert it to a number

            Arguments:
                table {dictionary} -- a look up table that map a type of calibration
                                      to its bit mask.(defined in AD9375-UG992)
                cal_args {string} --  string arguments from user in form of "CAL1|CAL2|CAL3"
                                      or "CAL1 CAL2 CAL3"  or "CAL1;CAL2;CAL3"

            Returns:
                int -- calibration value bit mask.
            """
            value = 0
            try:
                return int(cal_args, 0)
            except ValueError:
                pass
            for key in re.split(r'[;|\s]\s*', cal_args):
                value_tmp = table.get(key.upper())
                if (value_tmp) != None:
                    value |= value_tmp
                else:
                    self.log.warning(
                        "Calibration key `%s' is not in calibration table. "
                        "Ignoring this key.",
                        key.upper()
                    )
            return value
        ## Go, go, go!
        self.log.trace("Setting up RF CAL...")
        try:
            init_cals_mask = _parse_and_convert_cal_args(
                INIT_CALIBRATION_TABLE,
                args.get('init_cals', 'DEFAULT')
            )
            tracking_cals_mask = _parse_and_convert_cal_args(
                TRACKING_CALIBRATION_TABLE,
                args.get('tracking_cals', 'DEFAULT')
            )
            init_cals_timeout = int(
                args.get(
                    'init_cals_timeout',
                    str(self.mykonos.DEFAULT_INIT_CALS_TIMEOUT)
                ), 0
            )
        except ValueError as ex:
            self.log.warning("init() args missing or error using default \
                             value seeing following exception print out.")
            self.log.warning("{}".format(ex))
            init_cals_mask = _parse_and_convert_cal_args(
                INIT_CALIBRATION_TABLE, 'DEFAULT')
            tracking_cals_mask = _parse_and_convert_cal_args(
                TRACKING_CALIBRATION_TABLE, 'DEFAULT')
            init_cals_timeout = self.mykonos.DEFAULT_INIT_CALS_TIMEOUT
        self.log.debug("args[init_cals]=0x{:02X}".format(init_cals_mask))
        self.log.debug("args[tracking_cals]=0x{:02X}".format(tracking_cals_mask))
        async_exec(
            self.mykonos,
            "setup_cal",
            init_cals_mask,
            tracking_cals_mask,
            init_cals_timeout
        )


    def init_jesd(self, jesdcore, master_clock_rate, args):
        """
        Bring up the JESD link between Mykonos and the N310.
        All clocks must be set up and stable before starting this routine.
        """
        jesdcore.check_core()

        # JESD Lane Rate only depends on the master_clock_rate selection, since all
        # other link parameters (LMFS,N) remain constant.
        L = 4
        M = 4
        F = 2
        S = 1
        N = 16
        new_rate = master_clock_rate * M * N * (10.0/8) / L / S
        self.log.trace("Calculated JESD204b lane rate is {} Gbps".format(new_rate/1e9))
        self.mg_class.current_jesd_rate = \
            self.set_jesd_rate(
                jesdcore,
                new_rate,
                self.mg_class.current_jesd_rate)
        self.log.trace("Pulsing Mykonos Hard Reset...")
        self.mg_class.cpld.reset_mykonos()
        self.log.trace("Initializing Mykonos...")
        # TODO: If we can set the LO source after initialization, that would
        # enable us to switch LO sources without doing the entire JESD and
        # clocking bringup. For now, we'll keep it here, and every time the LO
        # source needs to be changed, we need to re-initialize (this is because
        # MYKONOS_initialize() takes in the entire device config, which includes
        # the LO source), but we can revisit this if we want to either
        # - speed up init when the only change is the LO source, or
        # - we want to make the LO source runtime-configurable.
        self.init_lo_source(args)
        self.mykonos.begin_initialization()
        # Multi-chip Sync requires two SYSREF pulses at least 17us apart.
        jesdcore.send_sysref_pulse()
        time.sleep(0.001) # 17us... ish.
        jesdcore.send_sysref_pulse()
        if args.get('tx_bw'):
            self.mykonos.set_bw_filter('TX', args.get('tx_bw'))
        if args.get('rx_bw'):
            self.mykonos.set_bw_filter('RX', args.get('rx_bw'))
        async_exec(self.mykonos, "finish_initialization")
        # According to the AD9371 user guide, p.57, the RF cal must come before
        # the framer/deframer init. We tried otherwise, and failed. So don't
        # move this anywhere else.
        self.init_rf_cal(args)
        self.log.trace("Starting JESD204b Link Initialization...")
        # Generally, enable the source before the sink. Start with the DAC side.
        self.log.trace("Starting FPGA framer...")
        jesdcore.init_framer()
        self.log.trace("Starting Mykonos deframer...")
        self.mykonos.start_jesd_rx()
        # Now for the ADC link. Note that the Mykonos framer will not start issuing CGS
        # characters until SYSREF is received by the framer. Therefore we enable the
        # framer in Mykonos and the FPGA, send a SYSREF pulse to everyone, and then
        # start the deframer in the FPGA.
        self.log.trace("Starting Mykonos framer...")
        self.mykonos.start_jesd_tx()
        jesdcore.enable_lmfc(True)
        jesdcore.send_sysref_pulse()
        # Allow a bit of time for SYSREF to reach Mykonos and then CGS to
        # appear. In several experiments this time requirement was only in the
        # 100s of nanoseconds.
        time.sleep(0.001)
        self.log.trace("Starting FPGA deframer...")
        jesdcore.init_deframer()

        # Allow a bit of time for CGS/ILA to complete.
        time.sleep(0.100)
        error_flag = False
        if not jesdcore.get_framer_status():
            self.log.error("JESD204b FPGA Core Framer is not synced!")
            error_flag = True
        if not self.check_mykonos_deframer_status():
            self.log.error("Mykonos JESD204b Deframer is not synced!")
            error_flag = True
        if not jesdcore.get_deframer_status():
            self.log.error("JESD204b FPGA Core Deframer is not synced!")
            error_flag = True
        if not self.check_mykonos_framer_status():
            self.log.error("Mykonos JESD204b Framer is not synced!")
            error_flag = True
        if (self.mykonos.get_multichip_sync_status() & 0xB) != 0xB:
            self.log.error("Mykonos Multi-chip Sync failed!")
            error_flag = True
        if error_flag:
            raise RuntimeError('JESD204B Link Initialization Failed. See MPM logs for details.')
        self.log.debug("JESD204B Link Initialization & Training Complete")


    def _full_init(self, slot_idx, master_clock_rate, ref_clock_freq, args):
        """
        Run the full initialization sequence. This will bring everything up
        from scratch: The LMK, JESD cores, the AD9371, calibrations, and
        anything else that is clocking-related.
        Depending on the settings, this can take a fair amount of time.
        """
        # Init some more periphs:
        # The following peripherals are only used during init, so we don't
        # want to hang on to them for the full lifetime of the Magnesium
        # class. This helps us close file descriptors associated with the
        # UIO objects.
        with open_uio(
            label="dboard-regs-{}".format(slot_idx),
            read_only=False
        ) as dboard_ctrl_regs:
            self.log.trace("Creating jesdcore object...")
            jesdcore = nijesdcore.NIJESDCore(dboard_ctrl_regs, slot_idx, **self.JESD_DEFAULT_ARGS)
            # Now get cracking with the actual init sequence:
            self.log.trace("Creating dboard clock control object...")
            db_clk_control = DboardClockControl(dboard_ctrl_regs, self.log)
            self.log.debug("Reset Dboard Clocking and JESD204B interfaces...")
            db_clk_control.reset_mmcm()
            jesdcore.reset()
            self.log.trace("Initializing LMK...")
            self.mg_class.lmk = self._init_lmk(
                self._spi_ifaces['lmk'],
                ref_clock_freq,
                master_clock_rate,
                self._spi_ifaces['phase_dac'],
                self.INIT_PHASE_DAC_WORD,
                self.PHASE_DAC_SPI_ADDR,
            )
            db_clk_control.enable_mmcm()
            # Synchronize DB Clocks
            self._sync_db_clock(
                dboard_ctrl_regs,
                master_clock_rate,
                ref_clock_freq,
                args)
            self.log.debug(
                "Sample Clocks and Phase DAC Configured Successfully!")
            # Clocks and PPS are now fully active!
            if args.get('skip_rfic', None) is None:
                async_exec(self.mykonos, "set_master_clock_rate", master_clock_rate)
                self.init_jesd(jesdcore, master_clock_rate, args)
            jesdcore = None # Help with garbage collection
            # That's all that requires access to the dboard regs!
        return True


    def init(self, args, old_args, fast_reinit):
        """
        Runs the actual initialization.

        Arguments:
        args -- Dictionary with user-specified args
        old_args -- Dictionary with user-specified args from the previous
                    initialization run.
        fast_reinit -- A hint to do a fast reinit. If nothing changes, then
                       we don't have to re-init everything and their dogs, we
                       can skip a whole bunch of things.
        """
        # If any of these changes, we need a full re-init:
        # TODO: This is not very DRY (because we're repeating default values),
        # and is generally smelly design. However, we're being super
        # conservative for now, because the only reliable reset sequence we
        # have for AD9371 is the full Monty. As we learn more about the chip,
        # we might be able to get away with a partial (fast) reinit even when
        # some of these values change.
        args_that_must_not_change = [
            ('rx_lo_source', 'internal'),
            ('tx_lo_source', 'internal'),
            ('init_cals', 'DEFAULT'),
            ('tracking_cals', 'DEFAULT'),
            ('init_cals_timeout', str(self.mykonos.DEFAULT_INIT_CALS_TIMEOUT)),
        ]
        if fast_reinit:
            for arg_key, arg_default in args_that_must_not_change:
                old_value = old_args.get(arg_key, arg_default)
                new_value = args.get(arg_key, arg_default)
                if old_value != new_value:
                    self.log.debug(
                        "The following init arg changed and caused "
                        "a full re-init sequence: {}".format(arg_key))
                    fast_reinit = False
            # TODO: Maybe we can switch to digital loopback without running the
            # initialization. For now, force init when rfic_digital_loopback is
            # set because we're being conservative.
            if 'rfic_digital_loopback' in args:
                self.log.debug("Using rfic_digital_loopback flag causes a "
                               "full re-init sequence.")
                fast_reinit = False
        # If we can't do fast re-init, start from scratch:
        if not fast_reinit:
            if not self._full_init(
                    self.mg_class.slot_idx,
                    self.mg_class.master_clock_rate,
                    self.mg_class.ref_clock_freq,
                    args
                ):
                return False
        else:
            self.log.debug("Running fast re-init with the following settings:")
            for arg_key, arg_default in args_that_must_not_change:
                self.log.debug(
                    "{}={}".format(arg_key, args.get(arg_key, arg_default)))
            return True
        if str2bool(args.get('rfic_digital_loopback')):
            self.log.warning(
                "RF Functionality Disabled: JESD204b digital loopback "
                "enabled inside Mykonos!")
            self.mykonos.enable_jesd_loopback(1)
        else:
            self.mykonos.start_radio()
        return True
