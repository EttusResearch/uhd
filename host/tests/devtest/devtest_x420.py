#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Run device tests for the x420."""

# pylint: disable=wrong-import-position
# pylint: disable=unused-import
# flake8: noqa
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


### The GPIO tests only work either with 100 Gbps NIC and 245.72 MHz master clock rate
### or with the X4_200 FPGA image which has resamplers enabled. If one of these conditions
### is not met, the GPIO tests will fail because streaming won't be stable and thus the 
### ATR states will flicker between the expected and the idle state. Therefore they are 
## disabled by default.

# from gpio_test import GpioTest
#
# GpioTest.tests = {}
# for port in ["GPIO0", "GPIO1"]:
#     for bank, driver in [("GPIOA", "DB0_RF0")]:
#         GpioTest.tests[f"{port}_{driver}"] = {
#             "addl_args": [
#                 "--src",
#                 " ".join([driver] * 12),
#                 "--bank",
#                 bank,
#                 "--port",
#                 port,
#                 "--bits",
#                 "12",
#             ],
#         }

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
    ("GPIOB", "DB1_RF0"),
]

from list_sensors_test import ListSensorsTest
from python_api_test import UhdPythonApiTest
from rx_multi_spc_timed_commands_test import RxMultiSpcTimedCommandsTest
from tx_multi_spc_timed_commands_test import TxMultiSpcTimedCommandsTest
