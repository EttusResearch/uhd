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

from tx_bursts_test import UhdTxBurstsTest
from test_pps_test import UhdTestPpsTest

from gpio_test import GpioX4xxSetGetSourceTest

GpioX4xxSetGetSourceTest.test_params = {
    "possible_sources": [
        "PS",
        "MPM",
        "USER_APP",
        "DB0_SPI",
        "DB0_RF0",
        "DB0_RF1",
        "DB1_SPI",
        "DB1_RF0",
        "DB1_RF1",
    ],
    "num_pins": 12,
}

from gpio_test import X4xxGpioPowerTest

from gpio_test import GpioAtrReadbackTest

GpioAtrReadbackTest.test_params = [
    ("GPIOA", "DB0_RF0"),
    ("GPIOA", "DB0_RF1"),
    ("GPIOB", "DB1_RF0"),
    ("GPIOB", "DB1_RF1"),
]

from bitbang_test import BitbangTest

BitbangTest.tests = {}
for port in ["GPIO0", "GPIO1"]:
    for bank, driver in [("GPIOA", "DB0_RF0"), ("GPIOB", "DB1_RF0")]:
        BitbangTest.tests[f"{port}_{driver}"] = {
            "addl_args": ["--bank", bank, "--port", port, "--src", " ".join([driver] * 12)]
        }

from list_sensors_test import ListSensorsTest
from python_api_test import UhdPythonApiTest
from rx_multi_spc_timed_commands_test import RxMultiSpcTimedCommandsTest
from tx_multi_spc_timed_commands_test import TxMultiSpcTimedCommandsTest

from trigger_test import TxTriggerTest
