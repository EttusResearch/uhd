#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Run device tests for the E31X series.
"""

# pylint: disable=wrong-import-position
# pylint: disable=unused-import
from usrp_probe_test import uhd_usrp_probe_test
from benchmark_rate_test import uhd_benchmark_rate_test
uhd_benchmark_rate_test.tests = {
    'mimo': {
        'duration': 1,
        'direction': 'tx,rx',
        'channels': '0,1',
        'rate': 1e6,
        'acceptable-underruns': 500,
    },
    'siso_chan0_slow': {
        'duration': 1,
        'direction': 'tx,rx',
        'chan': '0',
        'rate': 1e6,
        'acceptable-underruns': 50,
    },
    'siso_chan1_slow': {
        'duration': 1,
        'direction': 'tx,rx',
        'chan': '1',
        'rate': 1e6,
        'acceptable-underruns': 50,
    },
}

from rx_samples_to_file_test import rx_samples_to_file_test
rx_samples_to_file_test.tests = {
    'chan0': {
        'duration': 1,
        'subdev': 'A:0',
        'rate': 1e6,
    },
    'chan1': {
        'duration': 1,
        'subdev': 'A:1',
        'rate': 1e6,
    },
}

from tx_waveforms_test import uhd_tx_waveforms_test
uhd_tx_waveforms_test.tests = {
    'chan0': {
        'chan': '0',
    },
    'chan1': {
        'chan': '1',
    },
    'both_chans': {
        'chan': '0,1',
    },
}

from tx_bursts_test import uhd_tx_bursts_test
from test_pps_test import uhd_test_pps_test
