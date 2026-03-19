#!/usr/bin/env python3
#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""Measure IQ impairments and DC offsets and create compensation files.

This script is used to measure IQ impairments and DC offsets in USRP devices both in TX and RX
directions. The measured impairments are then used to create compensation files holding the I and
Q compensation coefficients, a group delay and the DC offsets. It is advised to run the script only
for a limited frequency range as the measurements can take a long time. While RX can be run
separately, RX will be included in TX measurements, too. But since measuring the TX impairments is
more complex, doing only RX will speed up the measurement if only RX is needed. When running a UHD
session with loaded compensation data, the compensation coefficients that are the closest to the
requested frequency will be used.
"""
import datetime
import json
import os
import time
from functools import lru_cache

import numpy as np
import uhd
from uhd.usrp import dram_utils

from . import database

# Check for scipy installation
try:
    from scipy.interpolate import CubicSpline
    from scipy.optimize import minimize_scalar
    from scipy.signal import freqz
except (ImportError, ModuleNotFoundError) as ex:
    raise ImportError(
        "The 'scipy' package is required for running IQ and DC correction measurements."
    ) from ex

# Check for tqdm installation
try:
    import tqdm
except ImportError as ex:
    raise ImportError(
        "The 'tqdm' package is required for running IQ and DC correction measurements."
    ) from ex


# Constants
TX_OFFSET_FREQUENCY = 3.1315e6
WFM_UPLOAD_RETRIES = 4
# The pattern of the log files for measurements
LOGFILE_PATTERN = "measurements_{}.json"
# Discard initial RX samples to allow the RX signal chain to settle after stream start.
RX_SAMPLES_TO_DISCARD = 10000

# FIXME: Use a common place for these address constants in combination with the C++ code in
# host/lib/include/uhdlib/usrp/dboard/hbx/hbx_constants.hpp
PERIPH_BASE = 0x80000
RF_CORE_WINDOW = 0xA000
IMP_TX_OFFSET = 0x0
NUM_COEFFS_REG = 0x0
GROUP_DELAY_REG_OFFSET = 0x4
IINLINE_COEFF_REG = 0x8
ICROSS_COEFF_REG = 0xC
QINLINE_COEFF_REG = 0x10
DC_TX_OFFSET = 0x40
DC_RX_OFFSET = 0x50
DC_CTRL_REG_OFFSET = 0x0
DC_VALUE_OFFSET = 0x4
RFDC_DC_CONV_FACTOR = 2**31


class DataCollection:
    """Class to hold measured impairments and DC measurements for a USRP device."""

    def __init__(self, module_serial, git_hash, external_lo):
        """Initialize the DataCollection object.

        Args:
        module_serial: USRP module serial number
        git_hash: Git hash of the UHD version used for the measurement
        external_lo: Boolean indicating whether an external LO was used for the measurement
        """
        self.module_serial = module_serial
        self.db_serial = -1
        self.git_hash = git_hash
        self.time = time.time()
        self.external_lo = external_lo
        self.freq_points = []

    def add_freq_point(self, point):
        """Point: FreqPoint object."""
        self.freq_points.append(point)

    def __str__(self):
        """String representation of the DataCollection object."""
        time_str = datetime.datetime.fromtimestamp(self.time).strftime("%Y-%m-%d %H:%M:%S")
        return f"DataCollection for {self.module_serial} (DB: {self.db_serial}) started at {time_str} (git hash={self.git_hash})"

    def to_json(self):
        """Convert the DataCollection object to a JSON serializable dictionary."""
        return {
            "module_serial": self.module_serial,
            "db_serial": self.db_serial,
            "git_hash": self.git_hash,
            "time": self.time,
            "external_lo": self.external_lo,
            "freq_points": [freq_point.to_json() for freq_point in self.freq_points],
        }

    def save_to_file(self, filename):
        """Save the DataCollection object to a JSON file."""
        with open(filename, "w") as f:
            json.dump(self.to_json(), f, indent=4)

    @staticmethod
    def from_json(obj):
        """Create a DataCollection object from a JSON object."""
        data_collection = DataCollection(
            obj["module_serial"], obj["git_hash"], obj.get("external_lo", False)
        )
        data_collection.time = obj["time"]
        for freq_point in obj["freq_points"]:
            data_collection.add_freq_point(FreqPoint.from_json(freq_point))
        return data_collection

    @staticmethod
    def load_from_file(filename):
        """Load the DataCollection object from a JSON file."""
        with open(filename, "r") as f:
            obj = json.load(f)
            return DataCollection.from_json(obj)


class FreqPoint:
    """Class to hold a baseband sweep measurement."""

    def __init__(self, lo_freq, direction="RX"):
        """Initialize a FreqPoint object.

        Args:
        lo_freq: LO frequency in Hz
        direction: "RX" or "TX" direction of the sweep
        """
        self.lo_freq = lo_freq
        self.direction = direction  # "RX" or "TX"
        self.information = {}
        self.dc_measurement = 0 + 0j
        self.iq_measurements = []

    def add_iq_measurement(self, iq_measurement):
        """Add an IQ Measurement object to the FreqPoint."""
        self.iq_measurements.append(iq_measurement)

    def to_json(self):
        """Convert the FreqPoint object to a JSON serializable dictionary."""
        return {
            "lo_freq": self.lo_freq,
            "direction": self.direction,
            "information": self.information,
            "dc_measurement": {
                "real": float(self.dc_measurement.real),
                "imag": float(self.dc_measurement.imag),
            },
            "iq_measurements": [measurement.to_json() for measurement in self.iq_measurements],
        }

    @staticmethod
    def from_json(obj):
        """Reads data form a JSON object and returns a FreqPoint object."""
        freq_point = FreqPoint(obj["lo_freq"], obj.get("direction", "RX"))
        if "gains" in obj:
            freq_point.information = obj["gains"]
        else:
            freq_point.information = obj["information"]
        freq_point.direction = obj.get("direction", "RX")
        dc_obj = obj.get("dc_measurement", {"real": 0, "imag": 0})
        freq_point.dc_measurement = complex(dc_obj.get("real", 0), dc_obj.get("imag", 0))
        for iq_measurement in obj["iq_measurements"]:
            freq_point.add_iq_measurement(IqMeasurement.from_json(iq_measurement))
        return freq_point


class IqMeasurement:
    """Class to hold a single measurement of IQ impairments."""

    def __init__(self, bb_freq, gain, phase):
        """Initialize a measurement object.

        Args:
        bb_freq: baseband frequency in Hz
        gain: gain imbalance in linear scale
        phase: phase skew in radians
        """
        self.bb_freq = bb_freq
        self.gain = gain
        self.phase = phase

    def to_json(self):
        """Convert the IqMeasurement object to a JSON serializable dictionary."""
        return {"bb_freq": self.bb_freq, "gain": self.gain, "phase": self.phase}

    @staticmethod
    def from_json(obj):
        """Create an IqMeasurement object from a JSON object."""
        return IqMeasurement(obj["bb_freq"], obj["gain"], obj["phase"])

    def __str__(self):
        """String representation of the IqMeasurement object."""
        phase_deg = np.degrees(self.phase)
        return f"Impairment: gain={self.gain:.3f}, phase={phase_deg:.3f} deg"


class CorrectionFilter:
    """Class to hold the correction filter coefficients for IQ compensation."""

    def __init__(
        self,
        rf_frequency,
        scaling_factor,
        icross_coefficients,
        qinline_coefficients,
        group_delay,
    ):
        """Initialize a CorrectionFilter object."""
        self.scaling_factor = scaling_factor
        self.icross_coefficients = icross_coefficients
        self.qinline_coefficients = qinline_coefficients
        self.group_delay = group_delay
        self.rf_frequency = rf_frequency


###################################################################################################
####### IQ and DC mathematical functions ##########################################################
###################################################################################################
# Cache size of 400 allows for 100 baseband frequencies (base frequency step of
# 10 MHz for Hafnium fits this number) with 4 different tones needed for RX and TX.
@lru_cache(maxsize=400)
def create_complex_sine(frequency, sample_rate, num_samples, amplitude=1.0):
    """Creates a complex sine wave signal with cached results."""
    normalized_frequency = 2 * np.pi * frequency / sample_rate
    phases = np.arange(num_samples) * normalized_frequency
    return np.exp(1j * phases) * amplitude


def calculate_impairments(signal, frequency_offset, sample_rate):
    """Calculates IQ impairments from a complex signal."""
    # apply Hanning window to the signal
    num_samples = int(len(signal))
    window = np.hanning(num_samples)
    signal = signal * window

    # create a complex sine wave at the frequency offset
    mixing_tone = create_complex_sine(-frequency_offset, sample_rate, num_samples)

    # mix real and imaginary part down to DC
    real_part_dc = np.real(signal) * mixing_tone
    imaginary_part_dc = np.imag(signal) * mixing_tone

    # average the parts
    real_part_avg = np.mean(real_part_dc)
    imaginary_part_avg = np.mean(imaginary_part_dc)

    # calculate the impairments from the parts
    gain_imbalance_lin = np.abs(imaginary_part_avg) / np.abs(real_part_avg)
    phase_diff_rad = np.angle(imaginary_part_avg) - np.angle(real_part_avg) + np.pi / 2
    phase_skew_rad = np.angle(np.exp(1j * phase_diff_rad))

    return IqMeasurement(frequency_offset, gain_imbalance_lin, phase_skew_rad)


def calculate_image_rejection(measurement):
    """Calculates image rejection from a measurement."""
    gain_phase_product = 2 * np.cos(measurement.phase) * measurement.gain
    doubled_gain_imbalance = measurement.gain**2
    nominator = 1 + doubled_gain_imbalance - gain_phase_product
    denominator = 1 + doubled_gain_imbalance + gain_phase_product
    return 10 * np.log10(nominator / denominator)


def calculate_q_magnitude(i_cross, q_inline):
    """Calculate the magnitude of Q path within the bandwidth based on the two FIR filters."""
    combined_fir = [q + 1j * i for i, q in zip(i_cross, q_inline)]
    num_frequencies = 4000
    _, h_combined = freqz(combined_fir, worN=num_frequencies)
    max_index = int(np.ceil(0.8 * num_frequencies))
    return np.max(np.abs(h_combined[:max_index]))


def calculate_correction_filter(freq_point, args):
    """Calculates the correction filter coefficients from a measurement."""
    # generate filters for each BasebandSweep
    # ------ single point compensation calculation ------
    # take the arithmetic mean of the gains and phases as the single point to compensate
    gains = np.array([m.gain for m in freq_point.iq_measurements])
    phases = np.array([m.phase for m in freq_point.iq_measurements])

    mean_gain = np.mean(gains)
    mean_phase = np.mean(phases)

    single_point_i_cross = -np.arctan(mean_phase)
    single_point_q_inline = 1 / (mean_gain * np.cos(mean_phase))

    # calculate the fractional filter part
    bb_frequencies = np.array([m.bb_freq for m in freq_point.iq_measurements])

    # Linear interpolation of phases over bb_frequencies
    # Perform a linear fit (1st degree polynomial) to the phases over bb_frequencies
    fit_coeffs = np.polyfit(bb_frequencies, phases, 1)
    interp_phases = np.polyval(fit_coeffs, bb_frequencies)

    # seek maximum frequency index and calculate time offset at this point
    max_freq_index = np.argmax(bb_frequencies)
    max_freq = bb_frequencies[max_freq_index]
    max_interp_phase = interp_phases[max_freq_index]
    time_offset_q = (max_interp_phase - mean_phase) / (2 * np.pi * max_freq)

    # ------ fractional delay calculation ------
    group_delay = args.num_coeffs // 2
    q_inline_filter = np.zeros(args.num_coeffs)
    for i in range(args.num_coeffs):
        q_inline_filter[i] = np.sinc((i - group_delay) - time_offset_q * args.sample_rate)

    # merge single point and fractional delay filter
    i_cross_filter = np.zeros(args.num_coeffs)
    i_cross_filter[group_delay] = single_point_i_cross
    q_inline_filter *= single_point_q_inline

    # scale filters to avoid clipping
    max_q_amplitude = calculate_q_magnitude(i_cross_filter, q_inline_filter)
    scaling_factor = np.min([1 / max_q_amplitude, 1])
    i_cross_filter *= scaling_factor
    q_inline_filter *= scaling_factor

    cfilter = CorrectionFilter(
        freq_point.lo_freq,
        scaling_factor,
        i_cross_filter.tolist(),
        q_inline_filter.tolist(),
        group_delay,
    )
    return cfilter


###################################################################################################
####### Helper functions ##########################################################################
###################################################################################################
def coefficient_to_fixed_point(coefficient):
    """Convert a coefficient to a fixed-point representation."""
    _frac_bits = 23
    _coeff_width = 25
    num = int(coefficient * (1 << _frac_bits)) & ((1 << _coeff_width) - 1)
    return num


def simple_rejection_calc(waveform, freq, args):
    """Calculate image rejection for a waveform."""
    image_power = cal_tone_power(waveform, -freq + TX_OFFSET_FREQUENCY, args.sample_rate)
    signal_power = cal_tone_power(waveform, freq + TX_OFFSET_FREQUENCY, args.sample_rate)
    return image_power - signal_power


def iq_to_dc_offset(complex_offset):
    """Converts a complex IQ offset to the format required for DC offset correction."""
    # The expectation is that the complex offset is in the range of -1 to 1 for I and Q.
    i_sample = int(complex_offset.real * (1 << 15))
    q_sample = int(complex_offset.imag * (1 << 15))
    return ((q_sample << 16) | (i_sample & 0xFFFF)) & 0xFFFFFFFF


def cal_tone_power(waveform, freq, sample_rate):
    """Calculate tone power for a waveform."""
    waveform_tone = waveform * create_complex_sine(
        -freq,
        sample_rate,
        len(waveform),
    )
    tone_power_db = 20 * np.log10(np.abs(np.mean(waveform_tone)) + 1e-12)
    return tone_power_db


def upload_waveform(tx, tx_waveform, mem_region=None):
    """Upload a waveform to the USRP with retries."""
    for _ in range(WFM_UPLOAD_RETRIES):
        try:
            tx.upload(tx_waveform, mem_regions=mem_region)
            break
        except RuntimeError:
            pass
    else:
        raise RuntimeError(f"Failed to upload TX waveform after {WFM_UPLOAD_RETRIES} retries.")


def get_log_filename(db_serial):
    """Get the log file name for a given DB serial number."""
    return LOGFILE_PATTERN.format(db_serial)


###################################################################################################
####### The actual measurement function ###########################################################
###################################################################################################


class HBXCompensator:
    """Class to handle the IQ & DC compensation measurement for X420 USRPs.

    Currently this is the only IQ & DC compensation class available. If other USRPs will be
    supported in the future, we should make this a parent class that the children can inherit
    from and only do their specific things.
    """

    HBX_BAND_LIMITS = (500e6, 2.9e9, 6e9, 9e9)
    # Available bandwidth up until freq x:
    # Up until 2.2 GHz: 200 MHz
    # Up until 20 GHz: 1 GHz
    HBX_BANDWIDTHS = {2.2e9: 200e6, 20e9: 1e9}

    def __init__(self, args, graph):
        """Initialize the HBXCompensator object."""
        self._args = args
        self._graph = graph
        self._storage = {}
        self._bb_sweep_tx = {}
        self._bb_sweep_rx = {}
        self._gain = {}
        self._phase = {}
        self._meas_tx = {}
        self._meas_rx = {}
        self._cal_data = None
        self._waveform_regions = None
        # Save if RJ45 connection is used (through ARM processor)
        self._rj45 = args.rj45
        # LO Radio Channel Pair in case an external LO is used
        self._lo_rcp = None
        # Create a map of channels to DB serials. It'll be filled as we go.
        self._channel_db_serial_map = {}
        # Internal variable for number of coefficients to avoid multiple reads
        self._fir_filter_num_coefficients = None
        self._waveform_num_samples = self._args.num_samples + RX_SAMPLES_TO_DISCARD

        if args.log:
            print(
                "Logging enabled. Measurement results will be saved to files "
                f"`{get_log_filename('<db_serial>')}` in current directory."
            )
            if not os.access(".", os.W_OK):
                raise RuntimeError(
                    "Current directory is not writable. Please choose a writable directory "
                    "for logging."
                )

        self._num_mbs = self._graph.get_num_mboards()
        self._ids = [
            self._graph.get_mb_controller(i).get_mboard_name() for i in range(self._num_mbs)
        ]
        # We have assured we're only dealing with x420s before returning the HBXCompensator,
        # so any further checks can be done on radio#0 only.
        self.comp_name = self._ids[0] + " IQ DC Compensation"

        # Get the mapping of channels to mboards
        channels = self._args.channels
        _radio = uhd.rfnoc.RadioControl(self._graph.get_block(self._graph.find_blocks("Radio")[0]))
        # Assuming radios are symmetric in terms of TX and RX ports
        self.channels_per_radio = _radio.get_num_input_ports()
        self.channels_per_mb = self.channels_per_radio * 2
        if max(channels) >= self.channels_per_mb * self._num_mbs:
            raise RuntimeError(
                f"Requested channel {max(channels)} but only channel number up to "
                f"{self.channels_per_mb * self._num_mbs - 1} available."
            )
        # We have two replay blocks per mboard. For a channel measurement we
        # use one for RX and one for TX. Thus from the overall channel list we
        # must only handle one channel per mboard per iteration.
        module_utilization = []
        for i in range(self._num_mbs):
            module_utilization.append(set(channels).intersection({i * 2, i * 2 + 1}))
        num_iterations = max(len(s) for s in module_utilization)
        self.ch_map = []
        for _ in range(num_iterations):
            self.ch_map.append([s.pop() for s in module_utilization if len(s) > 0])

        if self._args.lo_channel >= 0:
            if self._args.lo_channel >= self.channels_per_mb * self._num_mbs:
                raise RuntimeError(
                    f"Requested LO control channel {self._args.lo_channel} but only channel "
                    f"number up to {self.channels_per_mb * self._num_mbs - 1} available."
                )
            # Prepare LO RCP
            mboard_num = self._args.lo_channel // self.channels_per_mb
            radio_idx = self._args.lo_channel % self.channels_per_mb // self.channels_per_radio
            channel_idx = self._args.lo_channel % self.channels_per_mb % self.channels_per_radio
            self._lo_rcp = (
                uhd.rfnoc.RadioControl(self._graph.get_block(f"{mboard_num}/Radio#{radio_idx}")),
                channel_idx,
            )
            self._lo_rcp[0].set_tx_lo_export_enabled(True, "HBX_LO", self._lo_rcp[1])
            self._lo_rcp[0].set_rx_lo_export_enabled(True, "HBX_LO", self._lo_rcp[1])
            print(f"Using LO from {mboard_num}/Radio#{radio_idx}/Channel#{channel_idx}.")

    def run_measurement(self):
        """Run the IQ impairment compensation measurement."""
        # Preparing the DRAM utils, exact channel will be chosen later.
        for idx, channels in enumerate(self.ch_map):
            tx = {}
            rx = {}
            # Once we got here we only do a single channel per motherboard. Therefore we can
            # use one replay block for TX and one for RX without collision.
            print("Collecting basic information...")
            for ch in channels:
                board_num = ch // self.channels_per_mb
                tx[ch] = dram_utils.DramTransmitter(
                    self._graph, [f"{board_num}/Radio#0"], f"{board_num}/Replay#0"
                )
                rx[ch] = dram_utils.DramReceiver(
                    self._graph,
                    [f"{board_num}/Radio#0"],
                    f"{board_num}/Replay#1",
                    throttle="1.0",
                )
                module_serial = self._graph.get_mb_controller(board_num).get_eeprom()[
                    "module_serial"
                ]
                self._storage[ch] = DataCollection(
                    module_serial, uhd.get_version_string(), self._args.lo_channel >= 0
                )

            print(f"Starting measurement iteration {idx + 1} of {len(self.ch_map)}...")
            try:
                self._run_channel_measurement(self._graph, tx, rx, channels)
            finally:
                for ch in channels:
                    if self._args.log:
                        self._storage[ch].save_to_file(
                            get_log_filename(self._storage[ch].db_serial)
                        )
                    self._cal_data = {}
                    for freq_point in self._storage[ch].freq_points:
                        cfilter = calculate_correction_filter(freq_point, self._args)
                        self.add_cal_data_point(cfilter, freq_point, self._storage[ch].db_serial)
                    self.store()
            print("Saved IQ & DC compensation data to database.")

    def _run_channel_measurement(self, graph, tx, rx, channels):
        # Get one of the radios assuming that they are all equal
        radio = uhd.rfnoc.RadioControl(graph.get_block(graph.find_blocks("Radio")[0]))
        # The configured sample rate must equal the sample rate given, otherwise tell the user.
        device_sample_rate = radio.get_tick_rate()
        if device_sample_rate != self._args.sample_rate:
            raise RuntimeError(
                f"Configured sample rate {device_sample_rate} does not match given sample rate "
                f"{self._args.sample_rate}. Please choose a valid sample rate."
            )
        rf_frequencies, bb_frequencies = self._get_frequency_vectors(radio, self._args)

        self._prepare_iq_waveform_cache(tx, bb_frequencies, channels)

        print(
            f"Measuring {len(rf_frequencies)} RF frequencies between {rf_frequencies[0]/1e6:.1f} "
            f"MHz and {rf_frequencies[-1]/1e6:.1f} MHz for channels {channels}..."
        )
        pbar_max_value = len(rf_frequencies)

        # Bandwidth might be lower on the lower frequencies
        # To allow progress bar to show a good ETA estimate during IQ measurements,
        # the full baseband frequencies should be processed first -> reverse the
        # order of RF frequencies
        rf_frequencies = rf_frequencies[::-1]

        radio_channel_pairs = {}
        for ch in channels:
            mboard_num = ch // self.channels_per_mb
            radio_idx = ch % self.channels_per_mb // self.channels_per_radio
            channel_idx = ch % self.channels_per_mb % self.channels_per_radio
            print(
                f"Working on {mboard_num}/Radio#{radio_idx}/Channel#{channel_idx}, "
                f"overall channel {ch}..."
            )
            tx[ch].reconnect(graph, [f"{mboard_num}/Radio#{radio_idx}:{channel_idx}"])
            rx[ch].reconnect(graph, [f"{mboard_num}/Radio#{radio_idx}:{channel_idx}"])

            rcp = tx[ch].radio_chan_pairs[0]
            self._prepare_radio(rcp[0], rcp[1])
            radio_channel_pairs[ch] = rcp
            self._storage[ch].db_serial = "".join(chr(c) for c in rcp[0].get_db_eeprom()["serial"])

            # Start streaming waveform continously
            stream_cmd_tx = uhd.types.StreamCMD(uhd.types.StreamMode.start_cont)
            stream_cmd_tx.num_samps = self._waveform_num_samples
            stream_cmd_tx.stream_now = True
            dc_region = self._waveform_regions["dc"]
            tx_port = tx[ch].replay_ports[0]
            tx[ch].replay_blocks[0].config_play(dc_region[0], dc_region[1], tx_port)
            tx[ch].replay_blocks[0].issue_stream_cmd(stream_cmd_tx, tx_port)

        print("Measuring DC offsets")
        with tqdm.tqdm(
            total=pbar_max_value, unit=" RF frequencies", desc="Measuring DC offsets"
        ) as pbar:
            for center_freq in rf_frequencies:
                pbar.update(1)
                for ch in channels:
                    self._meas_rx[ch] = FreqPoint(center_freq, "RX")
                    self._storage[ch].add_freq_point(self._meas_rx[ch])
                    if self._args.tx:
                        self._meas_tx[ch] = FreqPoint(center_freq, "TX")
                        self._storage[ch].add_freq_point(self._meas_tx[ch])

                self._set_master_lo(center_freq)
                self._set_rf_freq_and_gain(radio_channel_pairs, center_freq)

                self._measure_dc_rx(rx, channels)
                if self._args.tx:
                    self._measure_dc_tx(tx, rx, channels)

            # stop all TX transmissions
            for ch in channels:
                stream_cmd_tx = uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont)
                tx[ch].issue_stream_cmd(stream_cmd_tx)
                radio = tx[ch].radio_chan_pairs[0][0]
                # Reset the DC offset as we don't want to have that during IQ measurements
                self.set_dc_offset(radio, False, tx=False)
                self.set_dc_offset(radio, False, tx=True)

            time.sleep(0.5)

        print("Measuring IQ impairments")

        pbar_max_value = len(rf_frequencies) * len(bb_frequencies)
        with tqdm.tqdm(
            total=pbar_max_value, unit=" BB frequencies", desc="Measuring IQ impairments"
        ) as pbar:

            iterators = {ch: iter(self._storage[ch].freq_points) for ch in channels}

            for center_freq in rf_frequencies:
                self._gain = {ch: -1 for ch in channels}
                self._phase = {ch: -1 for ch in channels}
                self._set_master_lo(center_freq)
                self._set_rf_freq_and_gain(radio_channel_pairs, center_freq)
                # Check if the bandwidth for the current frequency is smaller than what the
                # digital chain could do.
                _bw_limit = next(
                    bw for limit, bw in self.HBX_BANDWIDTHS.items() if center_freq <= limit
                )
                for ch in channels:
                    # We assume that the object exists because we have created it during DC
                    # measurements for all frequencies. If it doesn't exist, something went wrong.
                    self._meas_rx[ch] = next(iterators[ch])
                    self._store_gain_and_temp(radio_channel_pairs[ch], self._meas_rx[ch], True)

                    if self._args.tx:
                        # We assume that the object exists because we have created it during DC
                        # measurements for all freqs. If it doesn't exist, something went wrong.
                        self._meas_tx[ch] = next(iterators[ch])
                        self._meas_tx[ch].information = self._meas_rx[ch].information.copy()
                for i, freq in enumerate(bb_frequencies):
                    pbar.update(1)
                    if abs(freq) > _bw_limit / 2:
                        continue

                    waveform_mem_region = self._waveform_regions["rx"][i]
                    self._measure_iq_rx(tx, rx, freq, channels, waveform_mem_region)
                    if self._args.tx:
                        waveform_mem_region = self._waveform_regions["tx"][i]
                        self._measure_iq_tx(tx, rx, freq, channels, waveform_mem_region)

    def _prepare_iq_waveform_cache(self, txs, bb_frequencies, channels):
        """Generate and upload all IQ BB waveforms."""
        # skip if waveforms are already prepared
        if self._waveform_regions is not None:
            return

        print("Preparing and uploading IQ measurement waveforms to the device DRAM...")

        # precheck the device memory and waveform sizes
        # check if the waveforms are aligned with the word size
        waveform_num_bytes = self._waveform_num_samples * txs[channels[0]].bytes_per_sample
        if waveform_num_bytes % txs[channels[0]].word_size != 0:
            raise RuntimeError(
                f"Waveform size {waveform_num_bytes} bytes is not aligned with the word size "
                f"{txs[channels[0]].word_size} bytes. Please adjust the number of samples."
            )
        # check the total required memory for all waveforms against the
        # available memory on the device
        if self._args.tx:
            total_required_mem = (2 * len(bb_frequencies) + 1) * waveform_num_bytes
        else:
            total_required_mem = (len(bb_frequencies) + 1) * waveform_num_bytes
        replay_block_mem_size = txs[channels[0]].replay_blocks[0].get_mem_size()
        if total_required_mem > replay_block_mem_size:
            raise RuntimeError(
                "Not enough Replay DRAM to preload IQ measurement waveforms: "
                f"required {total_required_mem} bytes, available {replay_block_mem_size} bytes."
            )

        offset = 0
        rx_regions = []
        tx_regions = []
        dc_region = (offset, waveform_num_bytes)

        with tqdm.tqdm(
            total=len(bb_frequencies), unit=" waveforms", desc="Uploading waveforms"
        ) as pbar:

            # add zeros for DC measurement at the beginning of the memory
            dc_waveform = np.zeros(self._waveform_num_samples, dtype=np.complex64)
            offset += waveform_num_bytes
            for ch in channels:
                upload_waveform(txs[ch], dc_waveform, mem_region=dc_region)

            for freq in bb_frequencies:

                rx_waveform = create_complex_sine(
                    freq - TX_OFFSET_FREQUENCY,
                    self._args.sample_rate,
                    self._waveform_num_samples,
                ).astype(np.complex64)
                rx_regions.append((offset, waveform_num_bytes))
                offset += waveform_num_bytes
                for ch in channels:
                    upload_waveform(txs[ch], rx_waveform, mem_region=rx_regions[-1])

                if self._args.tx:
                    tx_waveform = create_complex_sine(
                        freq, self._args.sample_rate, self._waveform_num_samples
                    ).astype(np.complex64)
                    tx_regions.append((offset, waveform_num_bytes))
                    offset += waveform_num_bytes
                    for ch in channels:
                        upload_waveform(txs[ch], tx_waveform, mem_region=tx_regions[-1])

                pbar.update(1)

        self._waveform_regions = {"rx": rx_regions, "tx": tx_regions, "dc": dc_region}

    def _prepare_radio(self, radio, radio_channel):
        radio.set_tx_antenna("CAL_LOOPBACK", radio_channel)
        radio.set_rx_antenna("CAL_LOOPBACK", radio_channel)
        if self._args.lo_channel >= 0:
            radio.set_tx_lo_source("external", "HBX_LO", radio_channel)
            radio.set_rx_lo_source("external", "HBX_LO", radio_channel)
        else:
            radio.set_tx_lo_source("internal", "HBX_LO", radio_channel)
            radio.set_rx_lo_source("internal", "HBX_LO", radio_channel)

    def _get_frequency_vectors(self, radio, args):
        frequency_range = radio.get_rx_frequency_range(0)
        # Start correction only from 500 MHz on as below we use the converter in real mode.
        start_cal_range = 500e6

        # Use the set of frequencies within the allowed range
        if args.center_frequencies is not None:
            rf_frequencies = np.array(
                [
                    freq
                    for freq in args.center_frequencies
                    if start_cal_range <= freq <= frequency_range.stop()
                ]
            )
            if len(rf_frequencies) == 0:
                raise RuntimeError(
                    "No center frequencies within the allowed frequency range "
                    f"({start_cal_range/1e6:.1f} MHz - {frequency_range.stop()/1e6:.1f} MHz)."
                )
        else:
            # Determine the RF frequencies for a frequency sweep
            rf_start_freq = np.max([args.rf_start_frequency, start_cal_range])
            rf_end_freq = np.min([args.rf_end_frequency, frequency_range.stop()])
            rf_frequencies = np.arange(rf_start_freq, rf_end_freq + 0.0001, args.rf_frequency_step)
            # Find which HBX_BAND_LIMITS are within the rf_frequencies range
            min_rf = np.min(rf_frequencies)
            max_rf = np.max(rf_frequencies)
            hbx_band_limits_in_range = [
                freq for freq in self.HBX_BAND_LIMITS if min_rf <= freq <= max_rf
            ]
            # For each band limit in range, add the value and value+0.1 to get past the band limit
            hbx_band_limits_with_offsets = [
                val for freq in hbx_band_limits_in_range for val in (freq, freq + 0.1)
            ]
            # Now add those points, so we have values around the band limits.
            rf_frequencies = np.append(rf_frequencies, hbx_band_limits_with_offsets)
            rf_frequencies = np.unique(rf_frequencies)

        # Determine the baseband frequencies for a single RF frequency
        half_frequencies = np.arange(
            args.bb_frequency_step, args.sample_rate * 0.4 + 0.0001, args.bb_frequency_step
        )
        bb_frequencies = np.concatenate((-half_frequencies[::-1], half_frequencies))
        return rf_frequencies, bb_frequencies

    def _init_waveform_storage(self, args, num_bb_freqs):
        rx_waveforms = tx_waveforms = None
        if args.waveforms:
            rx_waveforms = np.zeros((num_bb_freqs, args.num_samples), dtype=np.complex64)
            if args.tx:
                tx_waveforms = np.zeros((num_bb_freqs, args.num_samples), dtype=np.complex64)
        return rx_waveforms, tx_waveforms

    def _set_center_frequency(self, radio_channel_pair, center_freq):
        radio, radio_channel = radio_channel_pair
        desired_tx_freq = tx_freq = center_freq + TX_OFFSET_FREQUENCY
        # If we move into the next band with our TX offset, our measurements won't fit the
        # actual compensation frequency anymore. In that case, set tx_freq to the center_freq
        # which would be the case if we hit the HBX overall frequency limit, too. Then the
        # offset will be applied at the RX in negative direction instead.
        if any(desired_tx_freq > limit >= center_freq for limit in self.HBX_BAND_LIMITS):
            tx_freq = center_freq
        tx_freq = radio.set_tx_frequency(tx_freq, radio_channel)

        tx_freq_diff = desired_tx_freq - tx_freq
        radio.set_rx_frequency(center_freq - tx_freq_diff, radio_channel)

    def _set_rf_freq_and_gain(self, radio_channel_pairs, center_freq):
        for ch in radio_channel_pairs:
            radio, radio_channel = radio_channel_pairs[ch]
            self._set_center_frequency(radio_channel_pairs[ch], center_freq)
            radio.set_rx_gain(46, radio_channel)
            if center_freq <= 6e9:
                radio.set_tx_gain(22, radio_channel)
            elif center_freq <= 9e9:
                radio.set_tx_gain(15, radio_channel)
            else:
                radio.set_tx_gain(29, radio_channel)

    def _set_master_lo(self, center_freq):
        if self._lo_rcp is not None:
            self._set_center_frequency(self._lo_rcp, center_freq)

    def _store_gain_and_temp(self, radio_channel_pair, freq_point_rx, include_temperature=False):
        radio = radio_channel_pair[0]
        radio_channel = radio_channel_pair[1]
        for gain_stage in radio.get_rx_gain_names(radio_channel):
            current_gain = radio.get_rx_gain(gain_stage, radio_channel)
            freq_point_rx.information[f"RX_{gain_stage}"] = current_gain
        for gain_stage in radio.get_tx_gain_names(radio_channel):
            current_gain = radio.get_tx_gain(gain_stage, radio_channel)
            freq_point_rx.information[f"TX_{gain_stage}"] = current_gain
        if include_temperature:
            temperature = radio.get_rx_sensor("temperature", radio_channel)
            freq_point_rx.information["temperature"] = temperature.to_int()

    ###############################################################################################
    ####### IQ Impairments methods ################################################################
    ###############################################################################################

    def set_tx_impairments_filter(self, radio, gain, phase):
        """Applies the TX filter coefficients to the radio."""
        # apply single point inversion matrix
        icross_factor = -np.arctan(phase)
        qinline_factor = 1 / (gain * np.cos(phase))
        # scale factors to avoid clipping
        complex_gain = np.abs(qinline_factor + 1j * icross_factor)
        scaling_factor = np.min([1 / complex_gain, 1])
        icross_factor *= scaling_factor
        qinline_factor *= scaling_factor

        # initialize FIR filter coefficients if not yet done
        if not self._fir_filter_num_coefficients:
            self._fir_filter_num_coefficients = radio.peek32(
                PERIPH_BASE + RF_CORE_WINDOW + IMP_TX_OFFSET + NUM_COEFFS_REG
            )
        num_coeffs = self._fir_filter_num_coefficients

        iinline_coeff = coefficient_to_fixed_point(scaling_factor)
        radio.poke32(
            PERIPH_BASE + RF_CORE_WINDOW + IMP_TX_OFFSET + IINLINE_COEFF_REG,
            iinline_coeff,
        )
        for i in reversed(range(num_coeffs)):
            qinline_coeff = coefficient_to_fixed_point(qinline_factor) if i == 0 else 0
            radio.poke32(
                PERIPH_BASE + RF_CORE_WINDOW + IMP_TX_OFFSET + QINLINE_COEFF_REG,
                qinline_coeff,
            )
            icross_coeff = coefficient_to_fixed_point(icross_factor) if i == 0 else 0
            radio.poke32(
                PERIPH_BASE + RF_CORE_WINDOW + IMP_TX_OFFSET + ICROSS_COEFF_REG,
                icross_coeff,
            )
        radio.poke32(PERIPH_BASE + RF_CORE_WINDOW + IMP_TX_OFFSET + GROUP_DELAY_REG_OFFSET, 0)

    def _measure(self, tx, rx, rx_waveform, waveform_mem_region):
        """Stream a finite TX waveform and capture the RX waveform at the same time."""
        stream_cmd_tx = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
        stream_cmd_tx.num_samps = len(rx_waveform)
        stream_cmd_tx.stream_now = True
        stream_cmd_rx = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
        stream_cmd_rx.num_samps = len(rx_waveform)
        stream_cmd_rx.stream_now = False
        stream_cmd_rx.trigger = uhd.types.StreamCMD.trigger_t.TX_RUNNING
        rx.issue_stream_cmd(stream_cmd_rx, wait_for_buffer_complete=False)
        tx_port = tx.replay_ports[0]
        tx.replay_blocks[0].config_play(waveform_mem_region[0], waveform_mem_region[1], tx_port)
        tx.replay_blocks[0].issue_stream_cmd(stream_cmd_tx, tx_port)
        # Follow mode cannot be used when connection via the ARM processor (RJ45) is used.
        if self._rj45:
            rx.wait_for_buffer_complete(stream_cmd_rx)
            rx.download(rx_waveform, follow_mode=False)
        else:
            rx.download(rx_waveform, follow_mode=True)

    def _measure_iq_rx(self, txs, rxs, freq, channels, waveform_mem_region):
        for ch in channels:
            tx = txs[ch]
            rx = rxs[ch]
            radio = tx.radio_chan_pairs[0][0]
            self.set_tx_impairments_filter(radio, 1, 0)
            rx_waveform = np.zeros(self._waveform_num_samples, dtype=np.complex64)
            self._measure(tx, rx, rx_waveform, waveform_mem_region)
            rx_measurement = calculate_impairments(
                rx_waveform[RX_SAMPLES_TO_DISCARD:], freq, self._args.sample_rate
            )
            self._meas_rx[ch].add_iq_measurement(rx_measurement)

    def _capture_and_compute_rejection(
        self, radio, tx, rx, rx_waveform_local, freq, gain, phase, waveform_mem_region
    ):
        """Prepare the TX filter, capture the waveform and compute the rejection."""
        self.set_tx_impairments_filter(radio, gain, phase)
        self._measure(tx, rx, rx_waveform_local, waveform_mem_region)
        return simple_rejection_calc(rx_waveform_local[RX_SAMPLES_TO_DISCARD:], freq, self._args)

    def _estimate_gain(
        self, radio, tx, rx, rx_waveform_local, freq, gain_options, phase, waveform_mem_region
    ):
        """Estimate gain for a fixed phase by sweeping gain options."""
        rejection = np.zeros(len(gain_options))
        for j, gain_option in enumerate(gain_options):
            rejection[j] = self._capture_and_compute_rejection(
                radio, tx, rx, rx_waveform_local, freq, gain_option, phase, waveform_mem_region
            )
        # find max rejection by interpolation and minimization
        interp_func = CubicSpline(gain_options, rejection, extrapolate=False)
        result = minimize_scalar(
            interp_func, bounds=(gain_options[0], gain_options[-1]), method="bounded"
        )
        if np.isclose(result.x, gain_options[0]) or np.isclose(result.x, gain_options[-1]):
            print(
                "Warning: Gain optimization hit bounds, results may be inaccurate at center "
                f"frequency {radio.get_rx_frequency(0)/1e6} MHz and baseband frequency "
                f"{freq/1e6} MHz."
            )
        return result.x

    def _estimate_phase(
        self, radio, tx, rx, rx_waveform_local, freq, gain, phase_options, waveform_mem_region
    ):
        """Estimate phase for a fixed gain by sweeping phase options."""
        rejection = np.zeros(len(phase_options))
        for j, phase_option in enumerate(phase_options):
            rejection[j] = self._capture_and_compute_rejection(
                radio, tx, rx, rx_waveform_local, freq, gain, phase_option, waveform_mem_region
            )
        # find max rejection by interpolation and minimization
        interp_func_phase = CubicSpline(phase_options, rejection, extrapolate=False)
        result = minimize_scalar(
            interp_func_phase,
            bounds=(phase_options[0], phase_options[-1]),
            method="bounded",
        )
        if np.isclose(result.x, phase_options[0]) or np.isclose(result.x, phase_options[-1]):
            print(
                "Warning: Phase optimization hit bounds, results may be inaccurate at center "
                f"frequency {radio.get_rx_frequency(0)/1e6} MHz and baseband frequency "
                f"{freq/1e6} MHz of radio {radio}."
            )
        return result.x

    def _measure_iq_tx(self, txs, rxs, freq, channels, waveform_mem_region):
        """Measure TX IQ impairments at a given baseband frequency.

        The image rejection is getting worse with gain and phase errors, when
        moving away from the optimal point. Therefore we first do a coarse
        sweep of gain and phase to find a good starting point. Then we do a
        finer sweep around the previously found optimum to trace the best gain
        and phase over frequency.
        """
        rx_waveform_local = np.zeros(self._waveform_num_samples, dtype=np.complex64)

        for ch in channels:
            tx = txs[ch]
            rx = rxs[ch]
            radio = tx.radio_chan_pairs[0][0]

            gain = self._gain[ch]
            phase = self._phase[ch]

            # initial estimate of gain and phase
            if gain == -1:
                gain_options = np.linspace(0.8, 1.2, 11)
                gain = self._estimate_gain(
                    radio, tx, rx, rx_waveform_local, freq, gain_options, 0, waveform_mem_region
                )
                phase_options_deg = np.linspace(-20, 20, 11)
                phase_options = np.radians(phase_options_deg)
                phase = self._estimate_phase(
                    radio,
                    tx,
                    rx,
                    rx_waveform_local,
                    freq,
                    gain,
                    phase_options,
                    waveform_mem_region,
                )
                # Initial gain estimate is very rough, so do a second pass with
                # a better phase estimate.
                gain_options = np.linspace(gain - 0.1, gain + 0.1, 11)
                gain = self._estimate_gain(
                    radio,
                    tx,
                    rx,
                    rx_waveform_local,
                    freq,
                    gain_options,
                    phase,
                    waveform_mem_region,
                )

            # refine gain and phase estimates for each frequency
            gain_options = np.linspace(gain - 0.03, gain + 0.03, 7)
            gain = self._estimate_gain(
                radio,
                tx,
                rx,
                rx_waveform_local,
                freq,
                gain_options,
                phase,
                waveform_mem_region,
            )
            phase_options_deg = np.linspace(np.degrees(phase) - 3, np.degrees(phase) + 3, 7)
            phase_options = np.radians(phase_options_deg)
            phase = self._estimate_phase(
                radio,
                tx,
                rx,
                rx_waveform_local,
                freq,
                gain,
                phase_options,
                waveform_mem_region,
            )

            # save tx measurement
            tx_measurement = IqMeasurement(freq, gain, phase)
            self._meas_tx[ch].add_iq_measurement(tx_measurement)

            # Save the gain and phase as starting point for the next iteration
            self._gain[ch] = gain
            self._phase[ch] = phase

    ###############################################################################################
    ####### DC Offset methods #####################################################################
    ###############################################################################################

    def set_dc_offset(self, radio, enable, dc_offset=0 + 0j, tx=True):
        """Applies the TX DC offset to the radio."""
        offset_base = DC_TX_OFFSET if tx else DC_RX_OFFSET

        # set value if needed
        if enable:
            raw_value = iq_to_dc_offset(dc_offset)
            radio.poke32(PERIPH_BASE + RF_CORE_WINDOW + offset_base + DC_VALUE_OFFSET, raw_value)

        # set status
        control_value = 1 if enable else 0
        radio.poke32(PERIPH_BASE + RF_CORE_WINDOW + offset_base + DC_CTRL_REG_OFFSET, control_value)

    def _capture(self, rx, rx_waveform):
        """Capture the RX waveform (while TX is running)."""
        stream_cmd_rx = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
        stream_cmd_rx.num_samps = len(rx_waveform)
        stream_cmd_rx.stream_now = True
        rx.issue_stream_cmd(stream_cmd_rx, wait_for_buffer_complete=False)
        # Follow mode cannot be used when connection via the ARM processor (RJ45) is used.
        if self._rj45:
            rx.wait_for_buffer_complete(stream_cmd_rx)
            rx.download(rx_waveform, follow_mode=False)
        else:
            rx.download(rx_waveform, follow_mode=True)

    def _measure_dc_rx(self, rxs, channels):
        for ch in channels:
            rx = rxs[ch]
            radio = rx.radio_chan_pairs[0][0]
            # Get DC offset cal from converters
            rfdc_dc_offset_i = (
                radio.get_tree()
                .access_double("/dboard/rx_frontends/0/dc_offset/i_conv_cal/value")
                .get()
            )
            rfdc_dc_offset_q = (
                radio.get_tree()
                .access_double("/dboard/rx_frontends/0/dc_offset/q_conv_cal/value")
                .get()
            )
            self.set_dc_offset(radio, False, tx=False)
            self.set_dc_offset(radio, False, tx=True)

            rx_waveform = np.zeros(self._waveform_num_samples, dtype=np.complex64)

            self._capture(rx, rx_waveform)
            dc_offset = np.mean(rx_waveform[RX_SAMPLES_TO_DISCARD:])
            dc_offset_i = dc_offset.real + rfdc_dc_offset_i / RFDC_DC_CONV_FACTOR
            dc_offset_q = dc_offset.imag + rfdc_dc_offset_q / RFDC_DC_CONV_FACTOR
            self._meas_rx[ch].dc_measurement = complex(dc_offset_i, dc_offset_q)
            self.set_dc_offset(radio, True, dc_offset, tx=False)

    def _estimate_offset(self, radio, rx, rx_waveform_local, offset_options, i_direction=True):
        """Estimate DC offset by sweeping candidate offsets."""
        dc_tone_power = np.zeros(len(offset_options))
        for j, offset_option in enumerate(offset_options):
            self.set_dc_offset(radio, True, offset_option, True)
            self._capture(rx, rx_waveform_local)
            dc_tone_power[j] = cal_tone_power(
                rx_waveform_local[RX_SAMPLES_TO_DISCARD:],
                TX_OFFSET_FREQUENCY,
                self._args.sample_rate,
            )
        # search in only a single direction
        x_values = offset_options.real if i_direction else offset_options.imag
        # find max rejection by interpolation and minimization
        interp_func = CubicSpline(x_values, dc_tone_power, extrapolate=False)
        result = minimize_scalar(interp_func, bounds=(x_values[0], x_values[-1]), method="bounded")
        if np.isclose(result.x, x_values[0]) or np.isclose(result.x, x_values[-1]):
            print(
                f"Warning: Offset optimization hit bounds, results may be inaccurate at center "
                f"frequency {radio.get_rx_frequency(0)/1e6} MHz."
            )
        if i_direction:
            return result.x + 1j * offset_options[0].imag
        else:
            return offset_options[0].real + 1j * result.x

    def _measure_dc_tx(self, txs, rxs, channels):
        """Measure TX DC offset.

        The LO leakage is getting worse with I and Q, when
        moving away from the optimal point. Therefore we can search for the
        optimal point by guessing a DC offset and finding the minimum.
        """
        rx_waveform_local = np.zeros(self._waveform_num_samples, dtype=np.complex64)

        for ch in channels:
            tx = txs[ch]
            rx = rxs[ch]
            radio = tx.radio_chan_pairs[0][0]

            # coarse phase
            i_options = np.linspace(-0.3, 0.3, 7)
            dc_options = i_options + 1j * 0
            coarse_i = self._estimate_offset(
                radio, rx, rx_waveform_local, dc_options, i_direction=True
            ).real

            q_options = np.linspace(-0.3, 0.3, 7)
            dc_options = coarse_i + 1j * q_options
            coarse_q = self._estimate_offset(
                radio, rx, rx_waveform_local, dc_options, i_direction=False
            ).imag

            # refine estimation
            i_options = np.linspace(coarse_i - 0.05, coarse_i + 0.05, 11)
            dc_options = i_options + 1j * coarse_q
            fine_i = self._estimate_offset(
                radio, rx, rx_waveform_local, dc_options, i_direction=True
            ).real

            q_options = np.linspace(coarse_q - 0.05, coarse_q + 0.05, 11)
            dc_options = fine_i + 1j * q_options
            fine_q = self._estimate_offset(
                radio, rx, rx_waveform_local, dc_options, i_direction=False
            ).imag

            # save tx measurement
            self._meas_tx[ch].dc_measurement = complex(fine_i, fine_q)

    def add_cal_data_point(self, cfilter, freq_point, db_serial):
        """Create the cal data to be written into the cal database."""
        # Currently, this script is only for X420 where we only have channel 0 on each
        # radio/daugtherboard. Therefore, we don't have that parameter in our
        # data structures but want to save it for the database:
        channel = 0
        # Make the measurements available per direction, channel and frequency:
        if db_serial not in self._cal_data:
            self._cal_data.update({db_serial: {"RX": {}, "TX": {}}})

        if channel not in self._cal_data[db_serial][freq_point.direction]:
            self._cal_data[db_serial][freq_point.direction].update(
                {channel: uhd.usrp.cal.IQDCCal(self.comp_name, db_serial, int(time.time()))}
            )

        self._cal_data[db_serial][freq_point.direction][channel].set_cal_coeff(
            cfilter.rf_frequency,
            cfilter.scaling_factor,
            cfilter.icross_coefficients,
            cfilter.qinline_coefficients,
            cfilter.group_delay,
            freq_point.dc_measurement.real,
            freq_point.dc_measurement.imag,
        )

    def store(self):
        """Store the compensation data in the database."""
        for serial, directions in self._cal_data.items():
            for direction in directions:
                for channel, data in directions[direction].items():
                    key = f"{direction.lower()}{channel}_rate{int(self._args.sample_rate)}_iq_dc"
                    database.write_cal_data(key, serial, data.serialize())


def get_usrp_iq_dc_compensator(args):
    """Get the USRP IQ DC compensator object that fits the device."""
    graph = uhd.rfnoc.RfnocGraph(
        f"{args.args}, master_clock_rate={args.sample_rate},ignore-cal-file=1"
    )
    num_mbs = graph.get_num_mboards()
    ids = [graph.get_mb_controller(i).get_mboard_name() for i in range(num_mbs)]

    if any(name != "x420" for name in ids):
        raise RuntimeError(
            f"Expected x420 device(s), but got {ids}. Unable to run the IQ "
            "impairment compensation."
        )
    print(f"=== Detected {num_mbs} USRP of type {ids[0]} ===")
    # For as long as only one product uses this we can go with this simple check.
    if ids[0] == "x420":
        return HBXCompensator(args, graph)
    else:
        raise RuntimeError(f"Unsupported USRP type: {ids[0]}. Currently only x420 is supported.")
