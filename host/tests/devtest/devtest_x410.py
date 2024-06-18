#
# Copyright 2015 Ettus Research LLC
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Run device tests for the x4x0 series."""

# pylint: disable=wrong-import-position
# pylint: disable=unused-import
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
        "rate": 4.096e6,
        "acceptable-underruns": 500,
        "tx_buffer": (0.1 * 12.288e6)
        + 32e6 * 8 * 1 / 32,  # 32 MB DRAM for each channel (32 bit OTW format),
        "rx_buffer": 0.1 * 12.288e6,
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

GpioTest.tests = {}
for port in ["GPIO0", "GPIO1"]:
    for bank, driver in [("GPIOA", "DB0_RF0")]:
        GpioTest.tests[f"{port}_{driver}"] = {
            "addl_args": [
                "--src",
                " ".join([driver] * 12),
                "--bank",
                bank,
                "--port",
                port,
                "--bits",
                "12",
            ],
        }

from gpio_test import GpioX4xxSetGetSourceTest

GpioX4xxSetGetSourceTest.test_params = {
    "possible_sources": [
        "PS",
        "MPM",
        "USER_APP",
        "DB0_RF0",
        "DB0_RF1",
        "DB0_SPI",
        "DB1_RF0",
        "DB1_RF1",
        "DB1_SPI",
    ],
    "num_pins": 12,
}

from gpio_test import X4xxGpioPowerTest

from bitbang_test import BitbangTest

BitbangTest.tests = {}
for port in ["GPIO0", "GPIO1"]:
    for bank, driver in [("GPIOA", "DB0_RF0"), ("GPIOB", "DB1_RF0")]:
        BitbangTest.tests[f"{port}_{driver}"] = {
            "addl_args": ["--bank", bank, "--port", port, "--src", " ".join([driver] * 12)]
        }

from gpio_test import GpioAtrReadbackTest

GpioAtrReadbackTest.test_params = [
    ("GPIOA", "DB0_RF0"),
    ("GPIOA", "DB0_RF1"),
    ("GPIOB", "DB1_RF0"),
    ("GPIOB", "DB1_RF1"),
]

from list_sensors_test import ListSensorsTest
from python_api_test import UhdPythonApiTest
from rx_multi_spc_timed_commands_test import RxMultiSpcTimedCommandsTest
from tx_multi_spc_timed_commands_test import TxMultiSpcTimedCommandsTest
