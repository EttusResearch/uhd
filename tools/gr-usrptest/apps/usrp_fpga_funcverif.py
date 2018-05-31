#!/usr/bin/env python
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Run functional verification tests. Note: Does not actually require GNU Radio.

This is basically a fancy executor for benchmark_rate.
"""

from __future__ import print_function
import os
import copy
import time
import argparse
import subprocess
from itertools import chain
# Python 2/3 compat hack
try:
    from itertools import izip
    zip = izip
except ImportError:
    pass
from six import iteritems

#### Settings dictionary ######################################################
# This stores all the settings for the individual tests. I hope it's obvious
# how to modify/amend/read it. Comments inline:
FUNCVERIF_SETTINGS = {
    # Every key corresponds to one target that can be run. The key is the
    # command line argument.
    'n310_1gige': {
        # These arguments will be passed to every run of benchmark_rate, unless
        # overriden.
        # Strings get expanded, so we can use Python string expansion here.
        # If something does not start with two dashes (--foo), then we keep it
        # as a string expansion argument. For example, the {master_clock_rate}
        # in the following string gets expanded from the master_clock_rate key/
        # value pair further down.
        '--args': "type=n3xx,addr={addr},master_clock_rate={master_clock_rate},{args}",
        '--seq-threshold': 0,
        '--drop-threshold': 0,
        '--underrun-threshold': 100,
        '--overrun-threshold': 100,
        '--rx_subdev': 'A:0 A:1 B:0 B:1',
        '--tx_subdev': 'A:0 A:1 B:0 B:1',
        '--duration': 60,
        # __tests is a special key, it contains a list of dicts, which in turn
        # describe the details of the test. len(__tests) equals the number of
        # tests that get executed.
        '__tests': [
            # Any command line argument (i.e., a key that starts with two
            # dashes) is also appended to the call to benchmark_rate. It will
            # override arguments that were listed above. This lets you, e.g.,
            # override --duration (see further down).
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 1,},
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 2,},
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 3,},

            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 1,},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 2,},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 3,},

            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 1,},
            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 2,},
            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 3,},

            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 1,},
            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 2,},
            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 3,},

            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 1,},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 2,},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 3,},

            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 1,},
            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 2,},
            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 3,},

            {'--rx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1',},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1',},
            {'--rx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1',},

            {'--rx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1,2',},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2',},
            {'--rx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2',},

            {'--rx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1,2,3',},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2,3',},
            {'--rx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2,3',},

            {'--tx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1',},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1',},
            {'--tx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1',},

            {'--tx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1,2',},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2',},
            {'--tx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2',},

            {'--tx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1,2,3',},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2,3',},
            {'--tx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2,3',},

            {'--rx_rate': 1.25e6,   '--tx_rate': 1.25e6, 'master_clock_rate':   '125e6',    '--channels': '0,1,2,3',},
            {'--rx_rate': 1.2288e6, '--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2,3',},
            {'--rx_rate': 1.536e6,  '--tx_rate': 1.536e6, 'master_clock_rate':  '153.6e6',  '--channels': '0,1,2,3',},
        ],
    },
    'n300_1gige': {
        '--args': "type=n3xx,addr={addr},master_clock_rate={master_clock_rate},{args}",
        '--seq-threshold': 0,
        '--drop-threshold': 0,
        '--underrun-threshold': 100,
        '--overrun-threshold': 100,
        '--rx_subdev': 'A:0 B:0',
        '--tx_subdev': 'A:0 B:0',
        '--duration': 60,
        '__tests': [
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 1,},

            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 1,},

            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 1,},

            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 1,},

            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 1,},

            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 1,},

            {'--rx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1',},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1',},
            {'--rx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1',},

            {'--tx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1',},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1',},
            {'--tx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1',},

            {'--rx_rate': 1.25e6,   '--tx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1',},
            {'--rx_rate': 1.2288e6, '--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1',},
            {'--rx_rate': 1.536e6,  '--tx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1',},
        ],
    },
    'n310_10gige': {
        '--args': "type=n3xx,addr={addr},master_clock_rate={master_clock_rate},{args}",
        '--seq-threshold': 0,
        '--drop-threshold': 0,
        '--underrun-threshold': 100,
        '--overrun-threshold': 100,
        '--rx_subdev': 'A:0 A:1 B:0 B:1',
        '--tx_subdev': 'A:0 A:1 B:0 B:1',
        '--duration': 60,
        '__tests': [
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 1,},
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 2,},
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 3,},

            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 1,},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 2,},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 3,},

            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 1,},
            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 2,},
            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 3,},

            {'--rx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--rx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 1,},
            {'--rx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 2,},
            {'--rx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 3,},

            {'--rx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--rx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 1,},
            {'--rx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 2,},
            {'--rx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 3,},

            {'--rx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--rx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 1,},
            {'--rx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 2,},
            {'--rx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 3,},

            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 1,},
            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 2,},
            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 3,},

            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 1,},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 2,},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 3,},

            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 1,},
            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 2,},
            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 3,},

            {'--tx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--tx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 1,},
            {'--tx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 2,},
            {'--tx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 3,},

            {'--tx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--tx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 1,},
            {'--tx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 2,},
            {'--tx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 3,},

            {'--tx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--tx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 1,},
            {'--tx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 2,},
            {'--tx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 3,},

            {'--rx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,2',},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,2',},
            {'--rx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,2',},

            {'--rx_rate': 125e6,    'master_clock_rate': '125e6',    '--channels': '0,2',},
            {'--rx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': '0,2',},
            {'--rx_rate': 153.6e6,  'master_clock_rate': '153.6e6',  '--channels': '0,2',},

            {'--rx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1,2',},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2',},
            {'--rx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2',},

            {'--rx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1,2,3',},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2,3',},
            {'--rx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2,3',},

            # | 2x TX         | 125e6             | 1.25e6, 12.5e6          | 60
            # | 2x TX         | 122.88e6          | 1.2288e6, 12.288e6      | 60
            # | 2x TX         | 153.6e6           | 1.536e6, 15.36e6        | 60
            {'--tx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,2',},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,2',},
            {'--tx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,2',},

            {'--tx_rate': 12.5e6,   'master_clock_rate': '125e6',    '--channels': '0,2',},
            {'--tx_rate': 12.288e6, 'master_clock_rate': '122.88e6', '--channels': '0,2',},
            {'--tx_rate': 15.36e6,  'master_clock_rate': '153.6e6',  '--channels': '0,2',},

            # | 3x TX         | 125e6             | 1.25e6                  | 60
            # | 3x TX         | 122.88e6          | 1.2288e6                | 60
            # | 3x TX         | 153.6e6           | 1.536e6                 | 60
            {'--tx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1,2',},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2',},
            {'--tx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2',},

            # | 4x RX         | 125e6             | 1.25e6, 62.5e6          | 60
            {'--rx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1,2',},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2',},
            {'--rx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2',},

            {'--rx_rate': 62.5e6,  'master_clock_rate': '125e6',    '--channels': '0,1,2',},
            {'--rx_rate': 61.44e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2',},
            {'--rx_rate': 76.8e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2',},
#
            {'--tx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1,2,3',},
            {'--tx_rate': 12.5e6,   'master_clock_rate': '125e6',    '--channels': '0,1,2,3',},

            # | 4x RX & 4x TX | 125e6             | 1.25e6, 62.5e6          | 60
            # | 4x RX & 4x TX | 122.88e6          | 1.2288e6, 61.44e6       | 60
            # | 4x RX & 4x TX | 153e6             | 1.536e6, 76.8e6         | 60
            {'--rx_rate': 1.25e6,   '--tx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1,2,3',},
            {'--rx_rate': 1.2288e6, '--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2,3',},
            {'--rx_rate': 1.536e6,  '--tx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2,3',},

            {'--rx_rate': 62.5e6,  '--tx_rate': 62.5e6,  'master_clock_rate': '125e6',    '--channels': '0,1,2,3', '--duration': 3600,},
            {'--rx_rate': 61.44e6, '--tx_rate': 61.44e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2,3', '--duration': 3600,},
            {'--rx_rate': 76.8e6,  '--tx_rate': 76.8e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2,3', '--duration': 3600,},
        ],
    },
    'n300_10gige': {
        '--args': "type=n3xx,addr={addr},master_clock_rate={master_clock_rate},{args}",
        '--seq-threshold': 0,
        '--drop-threshold': 0,
        '--underrun-threshold': 100,
        '--overrun-threshold': 100,
        '--rx_subdev': 'A:0 B:0',
        '--tx_subdev': 'A:0 B:0',
        '--duration': 60,
        '__tests': [
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--rx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 1,},

            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 1,},

            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--rx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 1,},

            {'--rx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--rx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 1,},

            {'--rx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--rx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 1,},

            {'--rx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--rx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 1,},

            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--tx_rate': 1.25e6, 'master_clock_rate': '125e6', '--channels': 1,},

            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': 1,},

            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--tx_rate': 1.536e6, 'master_clock_rate': '153.6e6', '--channels': 1,},

            {'--tx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 0,},
            {'--tx_rate': 125e6, 'master_clock_rate': '125e6', '--channels': 1,},

            {'--tx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 0,},
            {'--tx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': 1,},

            {'--tx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 0,},
            {'--tx_rate': 153.6e6, 'master_clock_rate': '153.6e6', '--channels': 1,},

            {'--rx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1',},
            {'--rx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1',},
            {'--rx_rate': 1.536e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1',},

            {'--rx_rate': 125e6,    'master_clock_rate': '125e6',    '--channels': '0,1',},
            {'--rx_rate': 122.88e6, 'master_clock_rate': '122.88e6', '--channels': '0,1',},
            {'--rx_rate': 153.6e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1',},

            {'--rx_rate': 1.25e6,   '--tx_rate': 1.25e6,   'master_clock_rate': '125e6',    '--channels': '0,1',},
            {'--rx_rate': 1.2288e6, '--tx_rate': 1.2288e6, 'master_clock_rate': '122.88e6', '--channels': '0,1',},
            {'--rx_rate': 1.536e6,    '--tx_rate': 1.536,  'master_clock_rate': '153.6e6',  '--channels': '0,1',},

            {'--rx_rate': 62.5e6,  '--tx_rate': 62.5e6,  'master_clock_rate': '125e6',    '--channels': '0,1',},
            {'--rx_rate': 61.44e6, '--tx_rate': 61.44e6, 'master_clock_rate': '122.88e6', '--channels': '0,1',},
            {'--rx_rate': 76.8e6,  '--tx_rate': 76.8e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1',},

            {'--rx_rate': 62.5e6,  '--tx_rate': 62.5e6,  'master_clock_rate': '125e6',    '--channels': '0,1', '--duration': 3600,},
            {'--rx_rate': 61.44e6, '--tx_rate': 61.44e6, 'master_clock_rate': '122.88e6', '--channels': '0,1', '--duration': 3600,},
            {'--rx_rate': 76.8e6,  '--tx_rate': 76.8e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1', '--duration': 3600,},
        ],
    },
    'n310_2x_10gige': {
        '--args': "type=n3xx,addr={addr},second_addr={second_addr},master_clock_rate={master_clock_rate},{args}",
        '--seq-threshold': 0,
        '--drop-threshold': 0,
        '--underrun-threshold': 100,
        '--overrun-threshold': 100,
        '--rx_subdev': 'A:0 A:1 B:0 B:1',
        '--tx_subdev': 'A:0 A:1 B:0 B:1',
        '--duration': 60,
        '__tests': [
            {'--rx_rate': 125e6,   '--tx_rate': 62.5e6,  'master_clock_rate': '125e6',    '--channels': '0,1,2,3',
             '--duration': 3600, '--underrun-threshold': 1000, '--overrun-threshold': 1000,},
            {'--rx_rate': 122.88e6,'--tx_rate': 61.44e6, 'master_clock_rate': '122.88e6', '--channels': '0,1,2,3',
             '--duration': 3600, '--underrun-threshold': 1000, '--overrun-threshold': 1000,},
            {'--rx_rate': 153.6e6, '--tx_rate': 76.8e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1,2,3',
             '--duration': 3600, '--underrun-threshold': 1000, '--overrun-threshold': 1000,},
        ],
    },
    'n300_2x_10gige': {
        '--args': "type=n3xx,addr={addr},second_addr={second_addr},master_clock_rate={master_clock_rate},{args}",
        '--seq-threshold': 0,
        '--drop-threshold': 0,
        '--underrun-threshold': 100,
        '--overrun-threshold': 100,
        '--rx_subdev': 'A:0 A:1 B:0 B:1',
        '--tx_subdev': 'A:0 A:1 B:0 B:1',
        '--duration': 60,
        '__tests': [
            {'--rx_rate': 125e6,   '--tx_rate': 62.5e6,  'master_clock_rate': '125e6',    '--channels': '0,1',
             '--duration': 3600, '--underrun-threshold': 1000, '--overrun-threshold': 1000,},
            {'--rx_rate': 122.88e6,'--tx_rate': 61.44e6, 'master_clock_rate': '122.88e6', '--channels': '0,1',
             '--duration': 3600, '--underrun-threshold': 1000, '--overrun-threshold': 1000,},
            {'--rx_rate': 153.6e6, '--tx_rate': 76.8e6,  'master_clock_rate': '153.6e6',  '--channels': '0,1',
             '--duration': 3600, '--underrun-threshold': 1000, '--overrun-threshold': 1000,},
        ],
    },
}


def parse_args():
    """Parse args."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        # "-d", "--device-type", choices=FUNCVERIF_SETTINGS.keys(),
        "device_type", choices=FUNCVERIF_SETTINGS.keys(),
        help="Device type (n310, ...)"
    )
    parser.add_argument(
        '-a', '--addr', help="IP Address"
    )
    parser.add_argument(
        '-2', '--second-addr', help="Second IP Address"
    )
    parser.add_argument(
        '-p', '--path', default='.', help="Path to examples",
    )
    parser.add_argument(
        '-n', '--dry-run', action='store_true', help="Dry run",
    )
    parser.add_argument(
        '-v', '--verbose', action='store_true', help="Increase verbosity",
    )
    parser.add_argument(
        '-f', '--print-fastpath', action='store_true',
        help="Don't disable fastpath messages",
    )
    parser.add_argument(
        '-s', '--sleep', type=int, default=30, help="Sleep time between tests",
    )
    parser.add_argument(
        '--args'
    )
    return parser.parse_args()


def run_benchmark_rate(bmr_args, cli_args):
    """
    Execute benchmark_rate
    """
    bmr_exe = os.path.join(cli_args.path, 'benchmark_rate')
    if not os.path.isfile(bmr_exe):
        print("ERROR: Could not find benchmark_rate!")
        exit(1)
    cmd = [bmr_exe] + bmr_args
    print("Executing: $ {}".format(" ".join(cmd)))
    exe_env = os.environ
    if not cli_args.verbose:
        exe_env['UHD_LOG_LEVEL'] = 'warning'
    if not cli_args.print_fastpath:
        exe_env['UHD_LOG_FASTPATH_DISABLE'] = '1'
    if cli_args.dry_run:
        return True
    if cli_args.verbose:
        return subprocess.call(cmd, env=exe_env) == 0
    # Non-Verbose mode is a bit more complicated:
    try:
        subprocess.check_output(cmd, env=exe_env)
        return True
    except subprocess.CalledProcessError as ex:
        print(ex.output)
        return False


def prepare_args(default_args, test_args, extra_keys):
    """
    Return a list of arguments for running benchmark_rate
    """
    extra_keys.update({
        key: str(val)
        for key, val in iteritems(test_args)
        if not key.startswith('--')
    })
    args = {
        key: str(val).format(**extra_keys)
        for key, val in iteritems(default_args)
        if key.startswith('--')
    }
    for key in test_args:
        if key.startswith('--'):
            args[key] = str(test_args[key]).format(**extra_keys)
    return list(chain.from_iterable(zip(args.keys(), args.values())))


def run_tests(args, settings):
    """
    args -- Command line args from parse_args
    settings -- Sub-dictionary from FUNCVERIF_SETTINGS
    """
    test_args_list = settings.pop('__tests')
    num_tests = len(test_args_list)
    default_args = {
        key: str(val)
        for key, val in iteritems(settings)
        if key.startswith('--')
    }
    extra_keys = {
        key: str(val)
        for key, val in iteritems(settings)
        if not key.startswith('--')
    }
    extra_keys.update({
        'addr': args.addr,
        'second_addr': args.second_addr,
        'args': args.args,
    })
    print("Preparing to execute {} tests...".format(num_tests))
    all_good = True
    for test_idx, test_args in enumerate(test_args_list):
        print("Running test {}/{}...".format(test_idx+1, num_tests))
        bmr_args = prepare_args(default_args, test_args, copy.copy(extra_keys))
        if not run_benchmark_rate(bmr_args, args):
            print("Failure!")
            all_good = False
        time.sleep(args.sleep)
    return all_good


def main():
    """Go, go, go!"""
    args = parse_args()
    settings = FUNCVERIF_SETTINGS[args.device_type]
    return run_tests(args, settings)


if __name__ == "__main__":
    exit(not main())

