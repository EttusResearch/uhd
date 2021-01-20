#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Run device tests for the X3x0 series.
"""

# pylint: disable=wrong-import-position
# pylint: disable=unused-import

import os
from usrp_probe import get_num_chans
num_chans = get_num_chans(os.getenv('_UHD_TEST_ARGS_STR', ""))
tx_chans = num_chans['tx']
rx_chans = num_chans['rx']

from benchmark_rate_test import uhd_benchmark_rate_test
uhd_benchmark_rate_test.tests = {}
for (rate, speed) in [(1e6, 'slow'), (25e6, 'fast')]:
    for chan in range(tx_chans):
        uhd_benchmark_rate_test.tests.update({
            'tx_chan{}_{}'.format(chan, speed): {
                'duration': 1,
                'direction': 'tx',
                'chan': str(chan),
                'rate': rate,
                'acceptable-underruns': 10,
                'tx_buffer': (0.1*1e6)+32e6*8*1/32,  # 32 MB DRAM for each channel (32 bit OTW format),
            }
        })
    for chan in range(rx_chans):
        uhd_benchmark_rate_test.tests.update({
            'rx_chan{}_{}'.format(chan, speed): {
                'duration': 1,
                'direction': 'rx',
                'chan': str(chan),
                'rate': rate,
                'rx_buffer': 0.1*1e6,
            }
        })
    if tx_chans > 0:
        all_chans_str = ",".join([str(chan) for chan in range(tx_chans)])
        uhd_benchmark_rate_test.tests.update({
            'all_tx_chans_{}'.format(speed): {
                'duration': 1,
                'direction': 'tx',
                'chan': all_chans_str,
                'rate': rate if speed == 'slow' else rate / tx_chans,
                'acceptable-underruns': 10,
                'tx_buffer': (0.1*1e6)+32e6*8*1/32,  # 32 MB DRAM for each channel (32 bit OTW format),
            }
        })
    if rx_chans > 0:
        all_chans_str = ",".join([str(chan) for chan in range(rx_chans)])
        uhd_benchmark_rate_test.tests.update({
            'all_rx_chans_{}'.format(speed): {
                'duration': 1,
                'direction': 'rx',
                'chan': all_chans_str,
                'rate': rate if speed == 'slow' else rate / rx_chans,
                'rx_buffer': 0.1*1e6,
            }
        })
    if tx_chans == rx_chans:
        for chan in range(tx_chans):
            uhd_benchmark_rate_test.tests.update({
                'siso_chan{}_{}'.format(chan, speed): {
                    'duration': 1,
                    'direction': 'tx,rx',
                    'chan': str(chan),
                    'rate': rate,
                    'acceptable-underruns': 10,
                    'tx_buffer': (0.1*1e6)+32e6*8*1/32,  # 32 MB DRAM for each channel (32 bit OTW format),
                    'rx_buffer': 0.1*1e6,
                }
            })
        all_chans_str = ",".join([str(chan) for chan in range(tx_chans)])
        uhd_benchmark_rate_test.tests.update({
            'mimo_{}'.format(speed): {
                'duration': 1,
                'direction': 'tx,rx',
                'chan': all_chans_str,
                'rate': rate if speed == 'slow' else rate / tx_chans,
                'acceptable-underruns': 500,
                'tx_buffer': (0.1*1e6)+32e6*8*1/32,  # 32 MB DRAM for each channel (32 bit OTW format),
                'rx_buffer': 0.1*1e6,
            }
        })

if (tx_chans > 0):
    from tx_waveforms_test import uhd_tx_waveforms_test
    uhd_tx_waveforms_test.tests = {}
    all_chans = []
    for i in range(tx_chans):
        all_chans.append(str(i))
        test_name = 'chan{}'.format(i)
        uhd_tx_waveforms_test.tests.update({test_name : {'chan': i}})
    if (tx_chans > 1):
        uhd_tx_waveforms_test.tests.update({'all_chans': {'chan': ",".join(all_chans)}})
    from tx_bursts_test import uhd_tx_bursts_test

if (rx_chans > 0):
    from rx_samples_to_file_test import rx_samples_to_file_test
from test_pps_test import uhd_test_pps_test
from gpio_test import gpio_test
from bitbang_test import bitbang_test
from list_sensors_test import list_sensors_test
from test_messages_test import test_messages_test

