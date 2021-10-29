#
# Copyright 2015 Ettus Research LLC
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Run device tests for the x4x0 series.
"""

# pylint: disable=wrong-import-position
# pylint: disable=unused-import
from benchmark_rate_test import uhd_benchmark_rate_test
uhd_benchmark_rate_test.tests = {
    'mimo_slow': {
        'duration': 1,
        'direction': 'tx,rx',
        'chan': '0,1',
        'rate': 1e6,
        'acceptable-underruns': 500,
        'tx_buffer': (0.1*1e6)+32e6*8*1/32,  # 32 MB DRAM for each channel (32 bit OTW format),
        'rx_buffer': 0.1*1e6,
    },
    'mimo_fast': {
        'duration': 1,
        'direction': 'tx,rx',
        'chan': '0,1',
        'rate': 4.096e6,
        'acceptable-underruns': 500,
        'tx_buffer': (0.1*12.288e6)+32e6*8*1/32,  # 32 MB DRAM for each channel (32 bit OTW format),
        'rx_buffer': 0.1*12.288e6,
    },
    'siso_chan0_slow': {
        'duration': 1,
        'direction': 'tx,rx',
        'chan': '0',
        'rate': 1e6,
        'acceptable-underruns': 10,
        'tx_buffer': (0.1*1e6)+32e6*8*1/32,  # 32 MB DRAM for each channel (32 bit OTW format),
        'rx_buffer': 0.1*1e6,
    },
    'siso_chan1_slow': {
        'duration': 1,
        'direction': 'tx,rx',
        'chan': '1',
        'rate': 1e6,
        'acceptable-underruns': 10,
        'tx_buffer': (0.1*1e6)+32e6*8*1/32,  # 32 MB DRAM for each channel (32 bit OTW format),
        'rx_buffer': 0.1*1e6,
    },
}

from tx_waveforms_test import uhd_tx_waveforms_test
uhd_tx_waveforms_test.tests = {
    'chan0': {
        'chan': '0',
    },
    'chan1': {
        'chan': '0',
    },
    'both_chans': {
        'chan': '0,1',
    },
}

from rx_samples_to_file_test import rx_samples_to_file_test
from tx_bursts_test import uhd_tx_bursts_test
from test_pps_test import uhd_test_pps_test

from gpio_test import gpio_test
gpio_test.tests = {}
for port in ["GPIO0", "GPIO1"]:
    for bank,driver in [("GPIOA", "DB0_RF0")]:
        gpio_test.tests[f"{port}_{driver}"] = {
            "addl_args": ["--src", " ".join([driver]*12), "--bank", bank, "--port", port, "--bits", "12"],
        }

from gpio_test import gpio_x410_set_get_source_test
gpio_x410_set_get_source_test.test_params = {
    "possible_sources": ["PS", "MPM", "USER_APP", "DB0_RF0", "DB0_RF1", "DB0_SPI", "DB1_RF0", "DB1_RF1", "DB1_SPI"],
    "num_pins": 12,
}

from gpio_test import x410_gpio_power_test

from bitbang_test import bitbang_test
bitbang_test.tests = {}
for port in ["GPIO0", "GPIO1"]:
    for bank,driver in [("GPIOA", "DB0_RF0"), ("GPIOB", "DB1_RF0")]:
        bitbang_test.tests[f"{port}_{driver}"] = {
            "addl_args": ["--bank", bank, "--port", port, "--src", " ".join([driver]*12)]
        }

from gpio_test import gpio_atr_readback_test
gpio_atr_readback_test.test_params = [
    ("GPIOA", "DB0_RF0"),
    ("GPIOA", "DB0_RF1"),
    ("GPIOB", "DB1_RF0"),
    ("GPIOB", "DB1_RF1"),
]

from list_sensors_test import list_sensors_test
from python_api_test import uhd_python_api_test
