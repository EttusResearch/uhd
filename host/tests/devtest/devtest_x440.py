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

from tx_bursts_test import uhd_tx_bursts_test
from test_pps_test import uhd_test_pps_test

from gpio_test import gpio_x4xx_set_get_source_test
gpio_x4xx_set_get_source_test.test_params = {
    "possible_sources": [
        "PS", "MPM", "USER_APP",
        "DB0_SPI", "DB0_RF0", "DB0_RF1",
        "DB1_SPI", "DB1_RF0", "DB1_RF1",
    ],
    "num_pins": 12,
}

from gpio_test import x4xx_gpio_power_test

from gpio_test import gpio_atr_readback_test
gpio_atr_readback_test.test_params = [
    ("GPIOA", "DB0_RF0"),
    ("GPIOA", "DB0_RF1"),
    ("GPIOB", "DB1_RF0"),
    ("GPIOB", "DB1_RF1"),
]

from bitbang_test import bitbang_test
bitbang_test.tests = {}
for port in ["GPIO0", "GPIO1"]:
    for bank, driver in [("GPIOA", "DB0_RF0"), ("GPIOB", "DB1_RF0")]:
        bitbang_test.tests[f"{port}_{driver}"] = {
            "addl_args": ["--bank", bank, "--port", port, "--src", " ".join([driver]*12)]
        }

from list_sensors_test import list_sensors_test
from python_api_test import uhd_python_api_test
from rx_multi_spc_timed_commands_test import rx_multi_spc_timed_commands_test
from tx_multi_spc_timed_commands_test import tx_multi_spc_timed_commands_test
