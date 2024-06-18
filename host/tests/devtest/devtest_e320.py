#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Run device tests for the e320 series."""

# flake8: noqa

from benchmark_rate_test import UhdBenchmarkRateTest

UhdBenchmarkRateTest.tests = {
    "mimo_slow": {
        "duration": 1,
        "direction": "tx,rx",
        "chan": "0,1",
        "rate": 1e6,
        "acceptable-underruns": 500,
        "tx_buffer": (0.1 * 1e6)
        + 32e6 * 8 * 1 / 32,  # 32 MB DRAM for each channel (32 bit OTW format),
        "rx_buffer": 0.1 * 1e6,
    },
    "mimo_fast": {
        "duration": 1,
        "direction": "tx,rx",
        "chan": "0,1",
        "rate": 8e6,
        "acceptable-underruns": 500,
        "tx_buffer": (0.1 * 12.5e6)
        + 32e6 * 8 * 1 / 32,  # 32 MB DRAM for each channel (32 bit OTW format),
        "rx_buffer": 0.1 * 12.5e6,
    },
    "siso_chan0_slow": {
        "duration": 1,
        "direction": "tx,rx",
        "chan": "0",
        "rate": 1e6,
        "acceptable-underruns": 10,
        "tx_buffer": (0.1 * 1e6)
        + 32e6 * 8 * 1 / 32,  # 32 MB DRAM for each channel (32 bit OTW format),
        "rx_buffer": 0.1 * 1e6,
    },
    "siso_chan1_slow": {
        "duration": 1,
        "direction": "tx,rx",
        "chan": "1",
        "rate": 1e6,
        "acceptable-underruns": 10,
        "tx_buffer": (0.1 * 1e6)
        + 32e6 * 8 * 1 / 32,  # 32 MB DRAM for each channel (32 bit OTW format),
        "rx_buffer": 0.1 * 1e6,
    },
}

from tx_waveforms_test import UhdTxWaveformsTest

UhdTxWaveformsTest.tests = {
    "chan0": {
        "chan": "0",
    },
    "chan1": {
        "chan": "0",
    },
    "both_chans": {
        "chan": "0,1",
    },
}

from rx_samples_to_file_test import RxSamplesToFileTest
from tx_bursts_test import UhdTxBurstsTest
from test_pps_test import UhdTestPpsTest
from gpio_test import GpioTest
from bitbang_test import BitbangTest
from list_sensors_test import ListSensorsTest
from rx_multi_spc_timed_commands_test import RxMultiSpcTimedCommandsTest
from tx_multi_spc_timed_commands_test import TxMultiSpcTimedCommandsTest
