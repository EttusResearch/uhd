#
# Copyright 2015-2016 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
"""
Run device tests for the B2xx series.
"""
from usrp_probe_test import uhd_usrp_probe_test
from benchmark_rate_test import uhd_benchmark_rate_test
uhd_benchmark_rate_test.tests = {
    'mimo': {
        'duration': 1,
        'direction': 'tx,rx',
        'channels': ['0,1',],
        'sample-rates': [1e6],
        'products': ['B210',],
        'acceptable-underruns': 500,
    },
    'siso_chan0_slow': {
        'duration': 1,
        'direction': 'tx,rx',
        'chan': '0',
        'rate': 1e6,
        'acceptable-underruns': 50,
    },
    #'siso_chan0_fast': {
        #'duration': 1,
        #'direction': 'tx,rx',
        #'chan': '0',
        #'rate': 40e6,
        #'acceptable-underruns': 500,
    #},
    'siso_chan1_slow': {
        'duration': 1,
        'direction': 'tx,rx',
        'chan': '1',
        'rate': 1e6,
        'acceptable-underruns': 50,
        'products': ['B210',],
    },
    #'siso_chan1_fast': {
        #'duration': 1,
        #'direction': 'tx,rx',
        #'chan': '1',
        #'rate': 40e6,
        #'acceptable-underruns': 500,
        #'products': ['B210',],
    #},
}

from rx_samples_to_file_test import rx_samples_to_file_test
rx_samples_to_file_test.tests = {
    'default': {
        'duration': 1,
        'subdev': 'A:A',
        'rate': 5e6,
        'products': ['B210', 'B200',],
    },
}

from tx_bursts_test import uhd_tx_bursts_test
from test_pps_test import uhd_test_pps_test
from gpio_test import gpio_test

