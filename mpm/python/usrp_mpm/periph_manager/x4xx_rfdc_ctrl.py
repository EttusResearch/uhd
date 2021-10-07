#
# Copyright 2019 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X400 RFDC Control Module
"""

import ast
from collections import OrderedDict
from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm.periph_manager.x4xx_rfdc_regs import RfdcRegsControl
from usrp_mpm.rpc_server import no_rpc

# Map the interpolation/decimation factor to fabric words.
# Keys: is_dac (False -> ADC, True -> DAC) and factor
FABRIC_WORDS_ARRAY = { # [is_dac][factor]
    False: {0: 16, 1: 16, 2: 8, 4: 4, 8: 2}, # ADC
    True: {0: -1, 1: -1, 2: 16, 4: 8, 8: 4} # DAC
}

RFDC_DEVICE_ID = 0

class X4xxRfdcCtrl:
    """
    Control class for the X4xx's RFDC

    """
    # Label for RFDC UIO
    rfdc_regs_label = "rfdc-regs"
    # Describes the mapping of ADC/DAC Tiles and Blocks to DB Slot IDs
    # Follows the below structure:
    #   <slot_idx>
    #       'adc': [ (<tile_number>, <block_number>), ... ]
    #       'dac': [ (<tile_number>, <block_number>), ... ]
    RFDC_DB_MAP = [
        {
            'adc': [(0, 1), (0, 0)],
            'dac': [(0, 0), (0, 1)],
        },
        {
            'adc': [(2, 1), (2, 0)],
            'dac': [(1, 0), (1, 1)],
        },
    ]

    # Maps all possible master_clock_rate (data clk rate * data SPC) values to the
    # corresponding sample rate, expected FPGA decimation, whether to configure
    # the SPLL in legacy mode (which uses a different divider), and whether half-band
    # resampling is used.
    # Using an OrderedDict to use the first rates as a preference for the default
    # rate for its corresponding decimation.
    master_to_sample_clk = OrderedDict({
        #      MCR:    (SPLL, decimation, legacy mode, half-band resampling)
        122.88e6*4:    (2.94912e9, 2, False, False), # RF (1M-8G)
        122.88e6*2:    (2.94912e9, 2, False, True),  # RF (1M-8G)
        122.88e6*1:    (2.94912e9, 8, False, False), # RF (1M-8G)
        125e6*2:       (3.00000e9, 2, False, True),  # RF (1M-8G)
        125e6*4:       (3.00000e9, 2, False, False), # RF (1M-8G)
        200e6:         (3.00000e9, 4, True,  False), # RF (Legacy Mode)
    })


    def __init__(self, get_spll_freq, log):
        self.log = log.getChild('RFDC')
        self._get_spll_freq = get_spll_freq
        self._rfdc_regs = RfdcRegsControl(self.rfdc_regs_label, self.log)
        self._rfdc_ctrl = lib.rfdc.rfdc_ctrl()
        self._rfdc_ctrl.init(RFDC_DEVICE_ID)

        # Stores the last set value of the nco freq for each channel
        # Follows the below structure:
        #   <slot_id>
        #       'rx': [chan0_freq, chan1_freq]
        #       'tx': [chan0_freq, chan1_freq]
        self._rfdc_freq_cache = [
            {
                'rx': [0, 0],
                'tx': [0, 0],
            },
            {
                'rx': [0, 0],
                'tx': [0, 0],
            },
        ]

        self._cal_freeze_cache = {}

    @no_rpc
    def tear_down(self):
        """
        Removes any stored references to our owning X4xx class instance and
        destructs anything that must happen at teardown
        """
        self._get_spll_freq = None
        del self._rfdc_ctrl

    ###########################################################################
    # Public APIs (not available as MPM RPC calls)
    ###########################################################################
    @no_rpc
    def set_reset(self, reset=True):
        """
        Resets the RFDC FPGA components or takes them out of reset.
        """
        if reset:
            # Assert RFDC AXI-S, filters and associated gearbox reset.
            self._rfdc_regs.set_reset_adc_dac_chains(reset=True)
            self._rfdc_regs.log_status()
            # Assert Radio clock PLL reset
            self._rfdc_regs.set_reset_mmcm(reset=True)
            # Resetting the MMCM will automatically disable clock buffers
            return

        # Take upstream MMCM out of reset
        self._rfdc_regs.set_reset_mmcm(reset=False)

        # Once the MMCM has locked, enable driving the clocks
        # to the rest of the design. Poll lock status for up
        # to 1 ms
        self._rfdc_regs.wait_for_mmcm_locked(timeout=0.001)
        self._rfdc_regs.set_gated_clock_enables(value=True)

        # De-assert RF signal chain reset
        self._rfdc_regs.set_reset_adc_dac_chains(reset=False)

        # Restart tiles in XRFdc
        # All ADC Tiles
        if not self._rfdc_ctrl.reset_tile(-1, False):
            self.log.warning('Error starting up ADC tiles')
        # All DAC Tiles
        if not self._rfdc_ctrl.reset_tile(-1, True):
            self.log.warning('Error starting up DAC tiles')

        # Set sample rate for all active tiles
        active_converters = set()
        for db_idx, db_info in enumerate(self.RFDC_DB_MAP):
            db_rfdc_resamp, _ = self._rfdc_regs.get_rfdc_resampling_factor(db_idx)
            for converter_type, tile_block_set in db_info.items():
                for tile, block in tile_block_set:
                    is_dac = converter_type != 'adc'
                    active_converter_tuple = (tile, block, db_rfdc_resamp, is_dac)
                    active_converters.add(active_converter_tuple)
        for tile, block, resampling_factor, is_dac in active_converters:
            self._rfdc_ctrl.reset_mixer_settings(tile, block, is_dac)
            self._rfdc_ctrl.set_sample_rate(tile, is_dac, self._get_spll_freq())
            self._set_interpolation_decimation(tile, block, is_dac, resampling_factor)

        self._rfdc_regs.log_status()

        # Set RFDC NCO reset event source to analog SYSREF
        for tile, block, _, is_dac in active_converters:
            self._rfdc_ctrl.set_nco_event_src(tile, block, is_dac)


    @no_rpc
    def sync(self):
        """
        Multi-tile Synchronization on both ADC and DAC
        """
        # These numbers are determined from the procedure mentioned in
        # PG269 section "Advanced Multi-Tile Synchronization API use".
        adc_latency = 1228  # ADC delay in sample clocks
        dac_latency = 800   # DAC delay in sample clocks

        # Ideally, this would be a set to avoiding duplicate indices,
        # but we need to use a list for compatibility with the rfdc_ctrl
        # C++ interface (std::vector)
        adc_tiles_to_sync = []
        dac_tiles_to_sync = []

        rfdc_map = self.RFDC_DB_MAP
        for db_id in rfdc_map:
            for converter_type, tile_block_set in db_id.items():
                for tile, _ in tile_block_set:
                    if converter_type == 'adc':
                        if tile not in adc_tiles_to_sync:
                            adc_tiles_to_sync.append(tile)
                    else:  # dac
                        if tile not in dac_tiles_to_sync:
                            dac_tiles_to_sync.append(tile)

        self._rfdc_ctrl.sync_tiles(adc_tiles_to_sync, False, adc_latency)
        self._rfdc_ctrl.sync_tiles(dac_tiles_to_sync, True, dac_latency)

        # We expect all sync'd tiles to have equal latencies
        # Sets don't add duplicates, so we can use that to look
        # for erroneous tiles
        adc_tile_latency_set = set()
        for tile in adc_tiles_to_sync:
            adc_tile_latency_set.add(
                self._rfdc_ctrl.get_tile_latency(tile, False))
        if len(adc_tile_latency_set) != 1:
            raise RuntimeError("ADC tiles failed to sync properly")

        dac_tile_latency_set = set()
        for tile in dac_tiles_to_sync:
            dac_tile_latency_set.add(
                self._rfdc_ctrl.get_tile_latency(tile, True))
        if len(dac_tile_latency_set) != 1:
            raise RuntimeError("DAC tiles failed to sync properly")

    @no_rpc
    def get_default_mcr(self):
        """
        Gets the default master clock rate based on FPGA decimation
        """
        fpga_decimation, fpga_halfband = self._rfdc_regs.get_rfdc_resampling_factor(0)
        for master_clock_rate in self.master_to_sample_clk:
            _, decimation, _, halfband = self.master_to_sample_clk[master_clock_rate]
            if decimation == fpga_decimation and fpga_halfband == halfband:
                return master_clock_rate
        raise RuntimeError('No master clock rate acceptable for current fpga '
                           'with decimation of {}'.format(fpga_decimation))

    @no_rpc
    def get_dsp_bw(self):
        """
        Return the bandwidth encoded in the RFdc registers.

        Note: This is X4xx-specific, not RFdc-specific. But this class owns the
        access to RfdcRegsControl, and the bandwidth is strongly related to the
        RFdc settings.
        """
        return self._rfdc_regs.get_fabric_dsp_info(0)[0]

    @no_rpc
    def get_rfdc_resampling_factor(self, db_idx):
        """
        Returns a tuple resampling_factor, halfbands.

        See RfdcRegsControl.get_rfdc_resampling_factor().
        """
        return self._rfdc_regs.get_rfdc_resampling_factor(db_idx)

    @no_rpc
    def rfdc_restore_nco_freq(self):
        """
        Restores previously set RFDC NCO Frequencies
        """
        for slot_id, slot_info in enumerate(self._rfdc_freq_cache):
            for direction, channel_frequencies in slot_info.items():
                self.rfdc_set_nco_freq(direction, slot_id, 0, channel_frequencies[0])
                self.rfdc_set_nco_freq(direction, slot_id, 1, channel_frequencies[1])

    @no_rpc
    def rfdc_restore_cal_freeze(self):
        """
        Restores the previously set calibration freeze settings
        """
        for slot_id in [0, 1]:
            for tile_id, block_id, _ in self._find_converters(slot_id, "rx", "both"):
                if (tile_id, block_id) in self._cal_freeze_cache:
                    self._rfdc_ctrl.set_cal_frozen(
                        tile_id, block_id, 0
                    )
                    self._rfdc_ctrl.set_cal_frozen(
                        tile_id,
                        block_id,
                        self._cal_freeze_cache[(tile_id, block_id)]
                    )


    ###########################################################################
    # Public APIs that get exposed as MPM RPC calls
    ###########################################################################
    def rfdc_set_nco_freq(self, direction, slot_id, channel, freq):
        """
        Sets the RFDC NCO Frequency for the specified channel
        """
        converters = self._find_converters(slot_id, direction, channel)
        assert len(converters) == 1
        (tile_id, block_id, is_dac) = converters[0]

        if not self._rfdc_ctrl.set_if(tile_id, block_id, is_dac, freq):
            raise RuntimeError("Error setting RFDC IF Frequency")
        self._rfdc_freq_cache[slot_id][direction][channel] = freq

        return self._rfdc_ctrl.get_nco_freq(tile_id, block_id, is_dac)

    def rfdc_get_nco_freq(self, direction, slot_id, channel):
        """
        Gets the RFDC NCO Frequency for the specified channel
        """
        converters = self._find_converters(slot_id, direction, channel)
        assert len(converters) == 1
        (tile_id, block_id, is_dac) = converters[0]

        return self._rfdc_ctrl.get_nco_freq(tile_id, block_id, is_dac)

    ### ADC cal ###############################################################
    def set_cal_frozen(self, frozen, slot_id, channel):
        """
        Set the freeze state for the ADC cal blocks

        Usage:
        > set_cal_frozen <frozen> <slot_id> <channel>

        <frozen> should be 0 to unfreeze the calibration blocks or 1 to freeze them.
        """
        for tile_id, block_id, _ in self._find_converters(slot_id, "rx", channel):
            self._cal_freeze_cache[(tile_id, block_id)] = frozen
            self._rfdc_ctrl.set_cal_frozen(tile_id, block_id, frozen)

    def get_cal_frozen(self, slot_id, channel):
        """
        Get the freeze states for each ADC cal block in the channel

        Usage:
        > get_cal_frozen <slot_id> <channel>
        """
        return [
            1 if self._rfdc_ctrl.get_cal_frozen(tile_id, block_id) else 0
            for tile_id, block_id, is_dac in self._find_converters(slot_id, "rx", channel)
        ]

    def set_cal_coefs(self, channel, slot_id, cal_block, coefs):
        """
        Manually override calibration block coefficients. You probably don't need to use this.
        """
        self.log.trace(
            "Setting ADC cal coefficients for channel={} slot_id={} cal_block={}".format(
                channel, slot_id, cal_block))
        for tile_id, block_id, _ in self._find_converters(slot_id, "rx", channel):
            self._rfdc_ctrl.set_adc_cal_coefficients(
                tile_id, block_id, cal_block, ast.literal_eval(coefs))

    def get_cal_coefs(self, channel, slot_id, cal_block):
        """
        Manually retrieve raw coefficients for the ADC calibration blocks.

        Usage:
        > get_cal_coefs <channel, 0-1> <slot_id, 0-1> <cal_block, 0-3>
        e.g.
        > get_cal_coefs 0 1 3
        Retrieves the coefficients for the TSCB block on channel 0 of DB 1.

        Valid values for cal_block are:
        0 - OCB1 (Unaffected by cal freeze)
        1 - OCB2 (Unaffected by cal freeze)
        2 - GCB
        3 - TSCB
        """
        self.log.trace(
            "Getting ADC cal coefficients for channel={} slot_id={} cal_block={}".format(
                channel, slot_id, cal_block))
        result = []
        for tile_id, block_id, _ in self._find_converters(slot_id, "rx", channel):
            result.append(self._rfdc_ctrl.get_adc_cal_coefficients(tile_id, block_id, cal_block))
        return result

    ### DAC mux
    def set_dac_mux_data(self, i_val, q_val):
        """
        Sets the data which is muxed into the DACs when the DAC mux is enabled

        Usage:
        > set_dac_mux_data <I> <Q>
        e.g.
        > set_dac_mux_data 123 456
        """
        self._rfdc_regs.set_cal_data(i_val, q_val)

    def set_dac_mux_enable(self, channel, enable):
        """
        Sets whether the DAC mux is enabled for a given channel

        Usage:
        > set_dac_mux_enable <channel, 0-3> <enable, 1=enabled>
        e.g.
        > set_dac_mux_enable 1 0
        """
        self._rfdc_regs.set_cal_enable(channel, bool(enable))

    ### ADC thresholds
    def setup_threshold(self, slot_id, channel, threshold_idx, mode, delay, under, over):
        """
        Configure the given ADC threshold block.

        Usage:
        > setup_threshold <slot_id> <channel> <threshold_idx> <mode> <delay> <under> <over>

        slot_id: Slot ID to configure, 0 or 1
        channel: Channel on the slot to configure, 0 or 1
        threshold_idx: Threshold block index, 0 or 1
        mode: Mode to configure, one of ["sticky_over", "sticky_under", "hysteresis"]
        delay: In hysteresis mode, number of samples before clearing flag.
        under: 0-16384, ADC codes to set the "under" threshold to
        over: 0-16384, ADC codes to set the "over" threshold to
        """
        for tile_id, block_id, _ in self._find_converters(slot_id, "rx", channel):
            THRESHOLDS = {
                0: lib.rfdc.threshold_id_options.THRESHOLD_0,
                1: lib.rfdc.threshold_id_options.THRESHOLD_1,
            }
            MODES = {
                "sticky_over": lib.rfdc.threshold_mode_options.TRSHD_STICKY_OVER,
                "sticky_under": lib.rfdc.threshold_mode_options.TRSHD_STICKY_UNDER,
                "hysteresis": lib.rfdc.threshold_mode_options.TRSHD_HYSTERESIS,
            }
            if mode not in MODES:
                raise RuntimeError(
                    f"Mode {mode} is not one of the allowable modes {list(MODES.keys())}")
            if threshold_idx not in THRESHOLDS:
                raise RuntimeError("threshold_idx must be 0 or 1")
            delay = int(delay)
            under = int(under)
            over = int(over)
            assert 0 <= under <= 16383
            assert 0 <= over <= 16383
            self._rfdc_ctrl.set_threshold_settings(
                tile_id, block_id,
                lib.rfdc.threshold_id_options.THRESHOLD_0,
                MODES[mode],
                delay,
                under,
                over)

    def get_threshold_status(self, slot_id, channel, threshold_idx):
        """
        Read the threshold status bit for the given threshold block from the device.

        Usage:
        > get_threshold_status <slot_id> <channel> <threshold_idx>
        e.g.
        > get_threshold_status 0 1 0
        """
        return self._rfdc_regs.get_threshold_status(slot_id, channel, threshold_idx) != 0


    ###########################################################################
    # Private helpers (note: x4xx_db_iface calls into those)
    ###########################################################################
    def _set_interpolation_decimation(self, tile, block, is_dac, factor):
        """
        Set the provided interpolation/decimation factor to the
        specified ADC/DAC tile, block

        Only gets called from set_reset_rfdc().
        """
        # Map the interpolation/decimation factor to fabric words.
        # Keys: is_dac (False -> ADC, True -> DAC) and factor
        # Disable FIFO
        self._rfdc_ctrl.set_data_fifo_state(tile, is_dac, False)
        # Define fabric rate based on given factor.
        fab_words = FABRIC_WORDS_ARRAY[is_dac].get(int(factor))
        if fab_words == -1:
            raise RuntimeError('Unsupported dec/int factor in RFDC')
        # Define dec/int constant based on integer factor
        if factor == 0:
            int_dec = lib.rfdc.interp_decim_options.INTERP_DECIM_OFF
        elif factor == 1:
            int_dec = lib.rfdc.interp_decim_options.INTERP_DECIM_1X
        elif factor == 2:
            int_dec = lib.rfdc.interp_decim_options.INTERP_DECIM_2X
        elif factor == 4:
            int_dec = lib.rfdc.interp_decim_options.INTERP_DECIM_4X
        elif factor == 8:
            int_dec = lib.rfdc.interp_decim_options.INTERP_DECIM_8X
        else:
            raise RuntimeError('Unsupported dec/int factor in RFDC')
        # Update tile, block settings...
        self.log.debug(
            "Setting %s for %s tile %d, block %d to %dx",
            ('interpolation' if is_dac else 'decimation'),
            'DAC' if is_dac else 'ADC', tile, block, factor)
        if is_dac:
            # Set interpolation
            self._rfdc_ctrl.set_interpolation_factor(tile, block, int_dec)
            self.log.trace(
                "  interpolation: %s",
                self._rfdc_ctrl.get_interpolation_factor(tile, block).name)
            # Set fabric write rate
            self._rfdc_ctrl.set_data_write_rate(tile, block, fab_words)
            self.log.trace(
                "  Read words: %d",
                self._rfdc_ctrl.get_data_write_rate(tile, block, True))
        else: # ADC
            # Set decimation
            self._rfdc_ctrl.set_decimation_factor(tile, block, int_dec)
            self.log.trace(
                "  Decimation: %s",
                self._rfdc_ctrl.get_decimation_factor(tile, block).name)
            # Set fabric read rate
            self._rfdc_ctrl.set_data_read_rate(tile, block, fab_words)
            self.log.trace(
                "  Read words: %d",
                self._rfdc_ctrl.get_data_read_rate(tile, block, False))
        # Clear interrupts
        self._rfdc_ctrl.clear_data_fifo_interrupts(tile, block, is_dac)
        # Enable FIFO
        self._rfdc_ctrl.set_data_fifo_state(tile, is_dac, True)


    def _find_converters(self, slot, direction, channel):
        """
        Returns a list of (tile_id, block_id, is_dac) tuples describing
        the data converters associated with a given channel and direction.
        """
        if direction not in ('rx', 'tx', 'both'):
            self.log.error('Invalid direction "{}". Cannot find '
                           'associated data converters'.format(direction))
            raise RuntimeError('Invalid direction "{}". Cannot find '
                               'associated data converters'.format(direction))
        if str(channel) not in ('0', '1', 'both'):
            self.log.error('Invalid channel "{}". Cannot find '
                           'associated data converters'.format(channel))
            raise RuntimeError('Invalid channel "{}". Cannot find '
                               'associated data converters'.format(channel))
        data_converters = []
        rfdc_map = self.RFDC_DB_MAP[slot]

        if direction in ('rx', 'both'):
            if str(channel) == '0' or str(channel) == 'both':
                (tile_id, block_id) = rfdc_map['adc'][0]
                data_converters.append((tile_id, block_id, False))
            if str(channel) == '1' or str(channel) == 'both':
                (tile_id, block_id) = rfdc_map['adc'][1]
                data_converters.append((tile_id, block_id, False))
        if direction in ('tx', 'both'):
            if str(channel) == '0' or str(channel) == 'both':
                (tile_id, block_id) = rfdc_map['dac'][0]
                data_converters.append((tile_id, block_id, True))
            if str(channel) == '1' or str(channel) == 'both':
                (tile_id, block_id) = rfdc_map['dac'][1]
                data_converters.append((tile_id, block_id, True))
        return data_converters
