#
# Copyright 2019 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X400 RFDC Control Module
"""

import ast
import itertools
from dataclasses import dataclass
from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm.periph_manager.x4xx_rfdc_regs import RfdcRegsControl
from usrp_mpm.rpc_utils import no_rpc
from usrp_mpm.mpmutils import LogRuntimeError

RFDC_DEVICE_ID = 0

class X4xxRfdcCtrl:
    """
    Control class for the X4xx's RFDC.

    This class is agnostic to blocks used on the RFdc. Device
    specific mappings between ADC/DAC block and RF channels
    must be given during initialization so it works for all
    X4xx devices.
    """

    RFSOC_FIN_MIN = 102.40625e6
    RFSOC_FIN_MAX = 6554e6
    CONV_RATE_MIN = 1000e6
    CONV_RATE_MAX = 4096e6
    RFDC_RESAMPLER = (8, 4, 2)

    MMCM_INPUT_MIN = 10e6
    # The MMCM_INPUT_MAX is 933 MHz according to the spec, but we limit it to 64 MHz
    # to ensure our slowest MMCM output will be phase aligned to the reference.
    MMCM_INPUT_MAX = 64e6
    MMCM_FOUTMIN = 6.25e6
    MMCM_FOUTMAX = 775e6
    MMCM_VCO_MIN = 800e6
    MMCM_VCO_MAX = 1600e6
    MMCM_FB_MIN = 2
    MMCM_FB_MAX = 64
    MMCM_OD_MIN = 1
    MMCM_OD_MAX = 128

    @dataclass
    class ConverterInfo:
        """
        Helper class to cache converter information. Tile
        and block identify the converter where freq and
        cal_freeze are used to cache the last nco_freq
        and the calibration freeze state.
        """
        tile: int
        block: int
        nco_freq: float = None
        cal_freeze: int = 0

    # Label for RFDC UIO
    rfdc_regs_label = "rfdc-regs"

    def __init__(self, log):
        self.log = log.getChild('RFDC')
        self._rfdc_regs = RfdcRegsControl(self.rfdc_regs_label, self.log)
        self._rfdc_ctrl = lib.rfdc.rfdc_ctrl()
        self._rfdc_ctrl.init(RFDC_DEVICE_ID)
        self.log.debug(
            f"Using RFDC driver version {self._rfdc_ctrl.get_rfdc_version()}, "
            f"libmetal version {self._rfdc_ctrl.get_metal_version(True)}, "
            f"(compile-time version: {self._rfdc_ctrl.get_metal_version(False)})")
        if self._rfdc_ctrl.get_metal_version(True) != self._rfdc_ctrl.get_metal_version(False):
            self.log.warning(
                f"libmetal compile time version {self._rfdc_ctrl.get_metal_version(False)} "
                f"does not match library version {self._rfdc_ctrl.get_metal_version(False)}!")

        rinfo = self._rfdc_regs.get_rfdc_info(0)
        self.log.debug(
            f"Found bitfile with bandwidth: {rinfo['bw']} MHz, RX/TX chans: "
            f"{rinfo['num_rx_chans']}/{rinfo['num_tx_chans']}, extra resampling: "
            f"{rinfo['extra_resampling']}, RFDC SPC RX/TX: "
            f"{rinfo['spc_rx']}/{rinfo['spc_tx']}")

        adc_mapping, dac_mapping = self._rfdc_regs.get_converter_mapping()

        assert len(adc_mapping) == len(dac_mapping), "len of ADC/DAC mappings differ"

        channels_per_db = (len(list) for list in itertools.chain(adc_mapping, dac_mapping))
        self._channels_per_db = next(channels_per_db)

        assert all(self._channels_per_db == i for i in channels_per_db), \
            "all ADC/DAC board mappings must be of equal length"

        self._adc_convs = tuple(X4xxRfdcCtrl.ConverterInfo(tile, block)
            for (tile, block) in itertools.chain(*adc_mapping))
        self._dac_convs = tuple(X4xxRfdcCtrl.ConverterInfo(tile, block)
            for (tile, block) in itertools.chain(*dac_mapping))

        self.log.debug(f"Got assigned ADCs/DACs for {len(self._adc_convs)} channels "
            f"with {self._channels_per_db} channels per board.")
        self.log.debug(f"ADCs {[(conv.tile, conv.block) for conv in self._adc_convs]}")
        self.log.debug(f"DACs {[(conv.tile, conv.block) for conv in self._dac_convs]}")


        self._check_converters_enabled()

    ###########################################################################
    # Private helpers (note: x4xx_db_iface calls into those)
    ###########################################################################
    def _check_converters_enabled(self):
        """
        Check that all configured converters are actually enabled on the RFDC.
        Prints an enable map of all tiles first to help debugging.
        """
        self.log.debug("tile/block | ADC | DAC | Notes")
        self.log.debug("-----------+-----+-----+------")
        for tile, block in itertools.product(range(4), range(4)):
            notes = ""
            chan = [
                idx
                for idx, conv in enumerate(self._adc_convs)
                if conv.tile == tile and conv.block == block]
            if chan:
                notes += f"RX Channel {chan[0]}. "
            chan = [
                idx
                for idx, conv in enumerate(self._dac_convs)
                if conv.tile == tile and conv.block == block]
            if chan:
                notes += f"TX Channel {chan[0]}. "
            if tile == 0 and block == 0:
                notes += "MTS ref tile. "
            self.log.debug(f"{tile}/{block}        |"
                f"  {int(self._rfdc_ctrl.is_adc_enabled(tile, block))}  |"
                f"  {int(self._rfdc_ctrl.is_dac_enabled(tile, block))}  | {notes} ")

        for conv in self._adc_convs:
            if not self._rfdc_ctrl.is_adc_enabled(conv.tile, conv.block):
                raise RuntimeError(f"ADC converter for tile {conv.tile}/{conv.block} "
                    "is configured but not enabled.")
        for conv in self._dac_convs:
            if not self._rfdc_ctrl.is_dac_enabled(conv.tile, conv.block):
                raise RuntimeError(f"DAC converter for tile {conv.tile}/{conv.block} "
                    "is configured but not enabled.")

        # According to pg269, multi-tile synchronization uses tile 0/block 0 as
        # a reference, and therefore, those tiles need to be enabled even if
        # those specific tiles are not used in the current bitfile image.
        # Newer versions of the xrfdc driver specify a reference-tile argument,
        # so maybe this requirement will be dropped in the future, which could
        # be useful when we do dual rates.
        # For now, we enable tile 0 / block 0 in all FPGA images. This is just
        # a double-check for safety and sanity.
        if not self._rfdc_ctrl.is_adc_enabled(0, 0):
            raise LogRuntimeError(
                self.log, f"ADC tile 0/block 0 is not enabled! MTS not possible!")
        if not self._rfdc_ctrl.is_dac_enabled(0, 0):
            raise LogRuntimeError(
                self.log, f"DAC tile 0/block 0 is not enabled! MTS not possible!")


    def _device_to_db_channel(self, device_channel):
        """
        Converts a device channel to a tuple of daughterboard channel and
        a daughterboard index.
        :param device_channel: channel number in device counting (one number over all boards)
        :return: tuple, first item channel in daughterboard counting, second daughterboard index
        """
        return (device_channel % self._channels_per_db, device_channel // self._channels_per_db)

    def _get_converter(self, direction, db_id, db_channel):
        """
        Returns a *single* converter that matches direction, db_id and conv_channel
        :param direction: converter type to search, "tx" for DACs, "rx" for ADCs
        :param db_id: index of daughterboard to search on
        :param db_channel: channel number on the board to search for
        :asserts: more than one converter found
        :return: matching converter (if any)
        """
        converters = {
            "tx": self._dac_convs,
            "rx": self._adc_convs,
        }.get(direction)
        if not converters:
            raise ValueError(f"Invalid direction '{direction}' given (chose between 'rx' or 'tx').")
        [converter, *residual] = [conv[1] for conv in
                self._filter_converters(db_id, db_channel, converters)]
        assert len(residual) == 0
        return converter

    def _find_converters(self, db_id, db_channel, direction):
        """
        Returns an iterable of converters that match the given parameter.
        :param db_id: index of daughterboard to search on ('all' for all daughterboards)
        :param db_channel: channel number on the board to search for ('all' for all channels)
        :param direction: converter type to search, "tx" for DACs, "rx" for ADCs, "both" for both
        :return: iterable of matching converters
        """
        if not direction in ("rx", "tx", "both"):
            raise ValueError("Allowed value for direction are: "
                f"'rx', 'tx', 'both'. Given: '{direction}'")
        rx_converters = []
        tx_converters = []
        if direction in ("rx", "both"):
            rx_converters = map(lambda conv: (conv[1].tile, conv[1].block, False),
                    self._filter_converters(db_id, db_channel, self._adc_convs))
        if direction in ("tx", "both"):
            tx_converters = map(lambda conv: (conv[1].tile, conv[1].block, True),
                    self._filter_converters(db_id, db_channel, self._dac_convs))
        return itertools.chain(rx_converters, tx_converters)

    def _check_valid_index(self, name, value):
        """
        Checks value to be either string or int. Value has to be an unsigned
        integer. If value is a string 'all' is allowed as well.
        :param name: The name of value (used for Exception messages)
        :param value: value to check
        :raises TypeError if not int or str, ValueError if not an unsigned integer or 'all'
        :return: validated value
        """
        if value == "both":
            # translate 'both' to 'all' for backward compat
            self.log.warning("'both' as glob is deprecated. Use 'all' instead.")
            value = "all"
        if isinstance(value, int):
            if value < 0:
                raise ValueError(f"{name} must not be negative")
        elif isinstance(value, str):
            # isdigit implies positive integer
            if not (value.isdigit() or (value == "all")):
                raise ValueError(f"{name} must denote an positive integer or 'all'")
        else:
            raise TypeError(f"{name} must be either 'int' or 'str'")

        return value

    def _filter_converters(self, db_id, db_channel, converters):
        """
        Returns an iterable on converters where items match db_id and db_channel.
        db_id and db_channel are allowed to be 'all' to match every
        daughterboard and/or every channel on the daughterboard.
        """
        db_id = self._check_valid_index("slot_id", db_id)
        db_channel = self._check_valid_index("channel", db_channel)
        def filter_expression(item):
            (conv_ch, conv_db) = self._device_to_db_channel(item[0])
            return (db_id == "all" or int(db_id) == conv_db) and \
                   (db_channel == "all" or int(db_channel) == conv_ch)
        return filter(filter_expression, enumerate(converters))

    def _set_interpolation_decimation(self, tile, block, is_dac, factor, fab_words):
        """
        Set the provided interpolation/decimation factor to the
        specified ADC/DAC tile, block

        Only gets called from set_reset_rfdc().
        """
        # Map the interpolation/decimation factor to fabric words.
        # Keys: is_dac (False -> ADC, True -> DAC) and factor
        # Disable FIFO
        self._rfdc_ctrl.set_data_fifo_state(tile, is_dac, False)
        if fab_words < 2:
            raise RuntimeError('Unsupported dec/int factor in RFDC')
        # Define dec/int constant based on integer factor
        int_dec = {
            0: lib.rfdc.interp_decim_options.INTERP_DECIM_OFF,
            1: lib.rfdc.interp_decim_options.INTERP_DECIM_1X,
            2: lib.rfdc.interp_decim_options.INTERP_DECIM_2X,
            4: lib.rfdc.interp_decim_options.INTERP_DECIM_4X,
            8: lib.rfdc.interp_decim_options.INTERP_DECIM_8X
        }.get(factor)
        if int_dec is None:
            raise RuntimeError(
                f'Unsupported dec/int factor of {factor} in RFDC')
        # Update tile, block settings...
        self.log.debug(
                "Setting %s for %s tile %d, block %d to %dx (SPC value: %d)",
            ('interpolation' if is_dac else 'decimation'),
            'DAC' if is_dac else 'ADC', tile, block, factor, fab_words)
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

    ###########################################################################
    # Public APIs (not available as MPM RPC calls)
    ###########################################################################
    @no_rpc
    def tear_down(self):
        """
        Removes any stored references to our owning X4xx class instance and
        destructs anything that must happen at teardown
        """
        # See PG269 chapter 4 "Bitstream Reconfiguration"
        self.shutdown_tiles()
        del self._rfdc_ctrl

    @no_rpc
    def reset_mmcm(self, reset=True, check_locked = True):
        """
        Resets the MMCM, or takes it out of reset.

        When taking it out of reset, it also waits until it is locked.

        Resetting the MMCM will automatically disable clock buffers
        """
        self._rfdc_regs.set_reset_mmcm(reset=reset)

        if reset:
            return

        if check_locked:
          # Once the MMCM has locked, enable driving the clocks to the rest of
          # the design. Poll lock status for up to 1 ms
          self._rfdc_regs.wait_for_mmcm_locked(timeout=0.001)
          self._rfdc_regs.set_gated_clock_enables(value=True)

    @no_rpc
    def reset_rfdc(self, reset=True):
        """
        Resets the RFDC FPGA components or takes them out of reset.

        (De-)Assert RFDC AXI-S, filters and associated gearbox reset.
        """
        self._rfdc_regs.set_reset_adc_dac_chains(reset=reset)
        if reset:
            self._rfdc_regs.log_status()

    @no_rpc
    def reset_gearboxes(self):
        """"
        Resets the ADC and DAC gearboxes.
        """
        self._rfdc_regs.reset_gearboxes()

    @no_rpc
    def reset_tiles(self):
        """
        This resets all ADC/DAC tiles.  All existing register settings are
        cleared and are replaced with the settings initially configured.
        """
        # All ADC Tiles
        if not self._rfdc_ctrl.reset_tile(-1, False):
            # We only have to worry about issues here if we see other issues like tiles not syncing
            self.log.trace('Error resetting ADC tiles. This is expected in most cases and "\
                           "startup_tiles() will usually compensate for this.')
        # All DAC Tiles
        if not self._rfdc_ctrl.reset_tile(-1, True):
            # We only have to worry about issues here if we see other issues like tiles not syncing
            self.log.trace('Error resetting DAC tiles. This is expected in most cases and "\
                           "startup_tiles() will usually compensate for this.')

    @no_rpc
    def startup_tiles(self, quiet=False):
        """
        PG269: This API function restarts the tile as requested through Tile_Id. If -1 is passed
        as Tile_Id, the function restarts all the enabled tiles. Existing register settings are
        not lost or altered in the process.
        """
        # Startup all ADC Tiles
        if not self._rfdc_ctrl.startup_tile(-1, False) and not quiet:
            self.log.warning('Error starting up ADC tiles')
        # Startup all DAC Tiles
        if not self._rfdc_ctrl.startup_tile(-1, True) and not quiet:
            self.log.warning('Error starting up DAC tiles')


    @no_rpc
    def shutdown_tiles(self):
        """
        PG269: This API function stops the tile as requested through Tile_Id. If -1 is passed as
        Tile_Id, the function stops all the enabled tiles. The existing register settings are not
        cleared.
        """
        # Shutdown all ADC Tiles
        if not self._rfdc_ctrl.shutdown_tile(-1, False):
            self.log.warning('Error shutting down ADC tiles')

        # Shutdown all DAC Tiles
        if not self._rfdc_ctrl.shutdown_tile(-1, True):
            self.log.warning('Error shutting down DAC tiles')

    @no_rpc
    def configure(self, ref_freq, rfdc_configs):
        """
        Configure RFDC settings (RFDC PLL, converters).
        """
        # Set sample rate for all active tiles
        converter_list = itertools.chain(
                map(lambda x: (x[1], x[0], True), enumerate(self._dac_convs)),
                map(lambda x: (x[1], x[0], False), enumerate(self._adc_convs)))

        for conv, channel, is_dac in converter_list:
            db_idx = self._device_to_db_channel(channel)[1]
            rfdc_config = rfdc_configs[db_idx]
            self._rfdc_ctrl.reset_mixer_settings(conv.tile, conv.block, is_dac)
            # Configure the RFDC PLL (either in bypass or PLL mode) and program
            # the real converter rate
            if not self._rfdc_ctrl.configure_pll(
                conv.tile,
                is_dac,
                int(ref_freq != rfdc_config.conv_rate),
                ref_freq,
                rfdc_config.conv_rate):
                self.log.error(f"Failed to configure RFDC PLL for {'DAC' if is_dac else 'ADC'} "
                                 f"at channel {channel}.")
                raise RuntimeError(f"Failed to configure RFDC PLL for {'DAC' if is_dac else 'ADC'} "
                                 f"at channel {channel}.")
            fab_words = self._rfdc_regs.get_rfdc_info(db_idx).get(
                    'spc_tx' if is_dac else 'spc_rx')
            self._set_interpolation_decimation(
                conv.tile, conv.block, is_dac, rfdc_config.resampling, fab_words)

        self._rfdc_regs.log_status()
        for conv, _, is_dac in converter_list:
            # Set RFDC NCO reset event source to analog SYSREF
            self._rfdc_ctrl.set_nco_event_src(conv.tile, conv.block, is_dac)
        # Now reset registers that might require updating
        self._rfdc_regs.reset()

    @no_rpc
    def determine_tile_latencies(self, db_idx):
        """
        This procedure is mentioned in PG269 section
        "Deterministic Multi-Tile Synchronization API Use". Quote:
        "To prevent this error, the Target_Latency value must first be determined
        for the user FIFO and tile configuration by running XRFdc_MultiConverter_Sync
        with the target set to -1."

        Then we add a margin.
        Quote from pg269: The margin value to be applied is
        specified in terms of sample clocks. For the RF-ADC tiles, this value
        must be a multiple of the number of FIFO read-words times the
        decimation factor, and for RF-DAC tiles this value can be a constant
        of 16.
        """
        adcs_to_sync = tuple(self._find_converters(db_idx, 'all', 'rx'))
        dacs_to_sync = tuple(self._find_converters(db_idx, 'all', 'tx'))
        adc_tiles_to_sync = tuple({x[0] for x in adcs_to_sync})
        dac_tiles_to_sync = tuple({x[0] for x in dacs_to_sync})

        # Per Xilinx input: "MTS needs to run a number of times so that we know
        # what the worst case mismatch is and we can apply a margin to it.", we
        # arbitrarily choose the number of attempts.
        num_training_attempts = 5
        adc_trained_latencies = []
        dac_trained_latencies = []
        # Run preliminary latency determination
        for _ in range(num_training_attempts):
            if not self._rfdc_ctrl.sync_tiles(adc_tiles_to_sync, False, -1):
                self.log.error("sync_tiles() failed to run for ADC latency determination.")
            if not self._rfdc_ctrl.sync_tiles(dac_tiles_to_sync, True, -1):
                self.log.error("sync_tiles() failed to run for DAC latency determination.")
            adc_trained_latencies.append(self._rfdc_ctrl.get_tile_latency(0, False))
            dac_trained_latencies.append(self._rfdc_ctrl.get_tile_latency(0, True))

        # We assume that all ADCs are running at the same decimation
        decimation = int(self._rfdc_ctrl.get_decimation_factor(
            adcs_to_sync[0][0],
            adcs_to_sync[0][1]))
        assert decimation
        spc_rx = self._rfdc_regs.get_rfdc_info(0)['spc_rx']
        safety_margin = 2 # This is a number we picked
        add_adc_margin = lambda latency: latency + decimation * spc_rx * safety_margin
        # We now read back the measured tile latencies and add margins as
        # described above
        adc_latencies = {
            tile_idx: add_adc_margin(max(adc_trained_latencies))
            for tile_idx in adc_tiles_to_sync
        }
        dac_latencies = {
            tile_idx: max(dac_trained_latencies) + 16
            for tile_idx in dac_tiles_to_sync
        }
        return adc_latencies, dac_latencies

    @no_rpc
    def set_tile_latencies(self, db_idx, adc_latency, dac_latency):
        """
        Apply an ADC latency values to all channels of a daughterboard.
        """
        adcs_to_sync = tuple(self._find_converters(db_idx, 'all', 'rx'))
        dacs_to_sync = tuple(self._find_converters(db_idx, 'all', 'tx'))
        adc_tiles_to_sync = tuple({x[0] for x in adcs_to_sync})
        dac_tiles_to_sync = tuple({x[0] for x in dacs_to_sync})
        if not self._rfdc_ctrl.sync_tiles(adc_tiles_to_sync, False, int(adc_latency)):
            self.log.warning("sync_tiles() failed to synchronize ADC tiles!")
        if not self._rfdc_ctrl.sync_tiles(dac_tiles_to_sync, True, int(dac_latency)):
            self.log.warning("sync_tiles() failed to synchronize DAC tiles!")

        # We expect all sync'd tiles to have equal latencies
        # check for both ADC and DAC separately
        latencies = [
            self._rfdc_ctrl.get_tile_latency(tile, False)
            for tile in adc_tiles_to_sync]
        if not all(latencies[0] == latency for latency in latencies):
            self.log.error("ADC tiles failed to sync properly.")
            raise RuntimeError("ADC tiles failed to sync properly.")
        if latencies[0] != adc_latency:
            self.log.warning(
                f"ADC latency failed to set to desired value (is: {latencies[0]}, "
                f"requested: {adc_latency}). This may cause problems in multi-device "
                f"synchronization.")

        latencies = [
            self._rfdc_ctrl.get_tile_latency(tile, True)
            for tile in dac_tiles_to_sync]
        if not all(latencies[0] == latency for latency in latencies):
            self.log.error("DAC tiles failed to sync properly.")
            raise RuntimeError("DAC tiles failed to sync properly.")
        if latencies[0] != dac_latency:
            self.log.warning(
                f"DAC latency failed to set to desired value (is: {latencies[0]}, "
                f"requested: {dac_latency}). This may cause problems in multi-device "
                f"synchronization.")

        return True

    @no_rpc
    def get_dsp_bw(self):
        """
        Return the bandwidth encoded in the RFdc registers.

        Note: This is X4xx-specific, not RFdc-specific. But this class owns the
        access to RfdcRegsControl, and the bandwidth is strongly related to the
        RFdc settings.
        """
        return self._rfdc_regs.get_rfdc_info(0)['bw']


    @no_rpc
    def get_dsp_info(self):
        """
        Return a dictionary of dictionaries, one per daughterboard, with the DSP
        settings that are baked into the FPGA.
        """
        return [self._rfdc_regs.get_rfdc_info(db_idx) for db_idx in range(2)]

    @no_rpc
    def rfdc_restore_nco_freq(self):
        """
        Restores previously set RFDC NCO Frequencies
        """
        for channel, (adc_conv, dac_conv) in enumerate(zip(self._adc_convs, self._dac_convs)):
            (conv_ch, conv_db) = self._device_to_db_channel(channel)
            if adc_conv.nco_freq:
                self.rfdc_set_nco_freq("rx", conv_db, conv_ch, adc_conv.nco_freq)
            if dac_conv.nco_freq:
                self.rfdc_set_nco_freq("tx", conv_db, conv_ch, dac_conv.nco_freq)


    @no_rpc
    def rfdc_restore_cal_freeze(self):
        """
        Restores the previously set calibration freeze settings
        """
        for conv in self._adc_convs:
            self._rfdc_ctrl.set_cal_frozen(conv.tile, conv.block, 0)
            self._rfdc_ctrl.set_cal_frozen(conv.tile, conv.block, conv.cal_freeze)

    @no_rpc
    def rfdc_configure_mmcm(self, input_div, fb_div, output_div_map):
        """
        Configures all dividers of the MMCM and starts the DRP
        """
        self.log.trace(
            f"Configure MMCM with Input_Div={input_div}, "
            f"mmcm_fb_div={fb_div} and output_div_map={output_div_map}.")
        assert self._rfdc_regs
        # Update cached values to reflect the state of the hardware and
        # propagate modifications accordingly.
        self._rfdc_regs.set_mmcm_div(input_div)
        self._rfdc_regs.set_mmcm_fb_div(fb_div)
        for clock_name, div_val in output_div_map.items():
            self._rfdc_regs.set_mmcm_output_div(div_val, clock_name)
        self._rfdc_regs.reconfigure_mmcm()

    @no_rpc
    def rfdc_update_mmcm_regs(self):
        """
        Update the saved state of the MMCM registers from the hardware
        """
        self._rfdc_regs.update_mmcm_regs()

    @no_rpc
    def enable_iq_swap(self, enable, db_idx, channel, is_dac):
        """
        Enable or disable swap of I and Q samples from the RFDCs.
        """
        self._rfdc_regs.enable_iq_swap(enable, db_idx, channel, is_dac)

    @no_rpc
    def get_converter_rate(self, db_idx, channel=0, is_dac=None):
        """
        Return the converter rate on a given daughterboard/channel.
        """
        # This can be extended such that 'None' means 'either tx or rx'. Should
        # we ever support boards with only tx or rx, then we can pick.
        is_dac = bool(is_dac)
        conv = self._get_converter('tx' if is_dac else 'rx', db_idx, channel)
        return self._rfdc_ctrl.get_sample_rate(conv.tile, conv.block, bool(is_dac))


    ###########################################################################
    # Public APIs that get exposed as MPM RPC calls
    ###########################################################################
    def rfdc_set_nco_freq(self, direction, slot_id, channel, freq):
        """
        Sets the RFDC NCO Frequency for the specified channel
        """
        conv = self._get_converter(direction, slot_id, channel)

        if not self._rfdc_ctrl.set_if(conv.tile, conv.block, direction == "tx", freq):
            raise RuntimeError("Error setting RFDC IF Frequency")
        conv.nco_freq = self._rfdc_ctrl.get_nco_freq(conv.tile, conv.block, direction == "tx")
        return conv.nco_freq

    def rfdc_get_nco_freq(self, direction, slot_id, channel):
        """
        Gets the RFDC NCO Frequency for the specified channel
        """
        conv = self._get_converter(direction, slot_id, channel)

        return self._rfdc_ctrl.get_nco_freq(conv.tile, conv.block, direction == "tx")

    ### ADC cal ###############################################################
    def set_calibration_mode(self, slot_id, channel, mode):
        """
        Set RFDC calibration mode
        """
        MODES = {
            "calib_mode1": lib.rfdc.calibration_mode_options.CALIB_MODE1,
            "calib_mode2": lib.rfdc.calibration_mode_options.CALIB_MODE2,
        }
        if mode not in MODES:
            raise RuntimeError(
                 f"Mode {mode} is not one of the allowable modes {list(MODES.keys())}.")
        for tile_id, block_id, _ in self._find_converters(slot_id, channel, "rx"):
            # Only set the cal-mode if it is not yet what we want.
            if self._rfdc_ctrl.get_calibration_mode(tile_id, block_id) != MODES[mode]:
                self.log.debug(f"Setting calibration mode {mode} "
                               f"for tile_id {tile_id}, block {block_id}.")
                self._rfdc_ctrl.set_calibration_mode(tile_id, block_id, MODES[mode])
                # As per PG269:
                self._rfdc_ctrl.startup_tile(tile_id, False)
                # Ensure the cal-mode was set properly, otherwise throw
                if self._rfdc_ctrl.get_calibration_mode(tile_id, block_id) != MODES[mode]:
                    self.log.error(
                        "RFDC error: Desired calibration mode could not be set properly "
                        f"for tile_id {tile_id}, block {block_id}.")
                    raise RuntimeError(
                        "RFDC error: Desired calibration mode could not be set properly "
                        f"for tile_id {tile_id}, block {block_id}.")
                self.log.debug(f"Successfully set calibration mode to {mode} "
                                   f"for tile_id {tile_id}, block {block_id}.")
            else:
                self.log.debug(f"Calibration mode {mode} for tile_id {tile_id}, "
                               f"block {block_id} already set, not changing it.")

    def set_cal_frozen(self, frozen, slot_id, channel):
        """
        Set the freeze state for the ADC cal blocks

        Usage:
        > set_cal_frozen <frozen> <slot_id> <channel>

        <frozen> should be 0 to unfreeze the calibration blocks or 1 to freeze them.
        """
        for _, conv in self._filter_converters(slot_id, channel, self._adc_convs):
            conv.cal_freeze = frozen
            self._rfdc_ctrl.set_cal_frozen(conv.tile, conv.block, frozen)

    def get_cal_frozen(self, slot_id, channel):
        """
        Get the freeze states for each ADC cal block in the channel

        Usage:
        > get_cal_frozen <slot_id> <channel>
        """
        def get_cal_frozen(conv):
            return 1 if self._rfdc_ctrl.get_cal_frozen(conv[1].tile, conv[1].block) else 0

        return list(map(get_cal_frozen,
            self._filter_converters(slot_id, channel, self._adc_convs)))

    def set_cal_coefs(self, channel, slot_id, cal_block, coefs):
        """
        Manually override calibration block coefficients. You probably don't need to use this.
        """
        self.log.trace("Setting ADC cal coefficients for "
            f"channel={channel} slot_id={slot_id} cal_block={cal_block}")
        for _, conv in self._filter_converters(slot_id, channel, self._adc_convs):
            self._rfdc_ctrl.set_adc_cal_coefficients(
                conv.tile, conv.block, cal_block, ast.literal_eval(coefs))

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
        def get_adc_dac_coeffs(conv):
            return self._rfdc_ctrl.get_adc_cal_coefficients(
                conv[1].tile, conv[1].block, cal_block)

        self.log.trace("Getting ADC cal coefficients for "
            "channel={channel} slot_id={slot_id} cal_block={cal_block}")
        return list(map(get_adc_dac_coeffs,
            self._filter_converters(slot_id, channel, self._adc_convs)))

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

    def set_dac_mux_enable(self, db_idx, channel, enable):
        """
        Sets whether the DAC mux is enabled for a given channel

        Usage:
        > set_dac_mux_enable <db_idx> <channel> <enable, 1=enabled>
        e.g.
        > set_dac_mux_enable 0 1 0
        """
        self._rfdc_regs.set_cal_enable(db_idx, channel, bool(enable))

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
        for _, conv in self._filter_converters(slot_id, channel, self._adc_convs):
            thresholds = {
                0: lib.rfdc.threshold_id_options.THRESHOLD_0,
                1: lib.rfdc.threshold_id_options.THRESHOLD_1,
            }
            modes = {
                "sticky_over": lib.rfdc.threshold_mode_options.TRSHD_STICKY_OVER,
                "sticky_under": lib.rfdc.threshold_mode_options.TRSHD_STICKY_UNDER,
                "hysteresis": lib.rfdc.threshold_mode_options.TRSHD_HYSTERESIS,
            }
            if mode not in modes:
                raise RuntimeError(
                    f"Mode {mode} is not one of the allowable modes {list(modes.keys())}")
            if threshold_idx not in thresholds:
                raise RuntimeError("threshold_idx must be 0 or 1")
            delay = int(delay)
            under = int(under)
            over = int(over)
            assert 0 <= under <= 16383
            assert 0 <= over <= 16383
            self._rfdc_ctrl.set_threshold_settings(
                conv.tile, conv.block,
                lib.rfdc.threshold_id_options.THRESHOLD_0,
                modes[mode],
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
        return self._rfdc_regs.get_threshold_status(
            slot_id, channel, threshold_idx) != 0
