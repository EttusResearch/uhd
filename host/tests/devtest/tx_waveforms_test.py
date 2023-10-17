#!/usr/bin/env python3
#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test using tx_waveforms """

import re
from uhd_test_base import uhd_example_test_case

class uhd_tx_waveforms_test(uhd_example_test_case):
    """
    Run tx_waveforms and check output.
    """
    tests = {}

    def setup_example(self):
        """
        Set args.
        """
        self.test_params = uhd_tx_waveforms_test.tests

    def run_test(self, test_name, test_args):
        """
        Runs tx_waveforms with the given parameters. Parses output and writes
        results to file.
        """
        samp_rate = test_args.get('rate', 1e6)
        nsamps = test_args.get('nsamps', '1000000')
        chan = test_args.get('chan', '0')
        freq = test_args.get('freq', '0')
        self.log.info('Running test %s, Channel = %s, Sample Rate = %f',
                      test_name, chan, samp_rate)
        args = [
            self.create_addr_args_str(),
            '--nsamps', str(nsamps),
            '--channels', str(chan),
            '--freq', str(freq),
            '--rate', str(samp_rate),
        ]
        (app, run_results) = self.run_example('tx_waveforms', args)
        # This only works if UHD_LOG_FASTPATH_DISABLE==0, which is usually not
        # the case. But we leave it in here just in case.
        run_results['num_late'] = int(len(re.findall(r'LLL', app.stderr)) * 3)
        run_results['passed'] = all([
            run_results['return_code'] == 0,
            run_results['num_late'] == 0,

        ])
        self.report_example_results(test_name, run_results)
        return run_results

