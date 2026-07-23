#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Run device tests for the B3xx series."""

# pylint: disable=wrong-import-position
# pylint: disable=unused-import
# flake8: noqa

from benchmark_rate_test import UhdBenchmarkRateTest

UhdBenchmarkRateTest.tests = {
    "tx_single_chan": {
        "duration": 1,
        "direction": "tx",
        "chan": "0",
        "rate": 1.92e6,
        "products": [
            "B310",
        ],
        "acceptable-underruns": 0,
        "acceptable-overruns": 0,
        "acceptable-D": 0,
        "acceptable-S": 0,
    },
    "tx_multi_chan": {
        "duration": 1,
        "direction": "tx",
        "chan": "0,1",
        "rate": 0.96e6,
        "products": [
            "B310",
        ],
        "acceptable-underruns": 0,
        "acceptable-overruns": 0,
        "acceptable-D": 0,
        "acceptable-S": 0,
    },
    "rx_single_chan": {
        "duration": 1,
        "direction": "rx",
        "chan": "0",
        "rate": 1.92e6,
        "products": [
            "B310",
        ],
        "acceptable-underruns": 0,
        "acceptable-overruns": 0,
        "acceptable-D": 0,
        "acceptable-S": 0,
    },
    "rx_multi_chan": {
        "duration": 1,
        "direction": "rx",
        "chan": "0,1",
        "rate": 0.96e6,
        "products": [
            "B310",
        ],
        "acceptable-underruns": 0,
        "acceptable-overruns": 0,
        "acceptable-D": 0,
        "acceptable-S": 0,
    },
    "tx_rx_single_chan": {
        "duration": 1,
        "direction": "tx,rx",
        "chan": "0",
        "rate": 1.92e6,
        "products": [
            "B310",
        ],
        "acceptable-underruns": 0,
        "acceptable-overruns": 0,
        "acceptable-D": 0,
        "acceptable-S": 0,
    },
}

from rx_samples_to_file_test import RxSamplesToFileTest

RxSamplesToFileTest.tests = {
    "chan0": {
        "duration": 1,
        "subdev": "A:0",
        "rate": 0.96e6,
    },
    "chan1": {
        "duration": 1,
        "subdev": "A:1",
        "rate": 0.96e6,
    },
}

from tx_waveforms_test import UhdTxWaveformsTest

UhdTxWaveformsTest.tests = {
    "chan0": {
        "chan": "0",
    },
    "chan1": {
        "chan": "1",
    },
    "both_chans": {
        "chan": "0,1",
    },
}

from list_sensors_test import ListSensorsTest
from python_api_test import UhdPythonApiTest
from test_pps_test import UhdTestPpsTest
from tx_bursts_test import UhdTxBurstsTest
from usrp_probe_test import UhdUsrpProbeTest
