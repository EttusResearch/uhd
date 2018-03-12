#!/usr/bin/env python
#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Run the test for tx_burst """

import re
from uhd_test_base import uhd_example_test_case

class uhd_tx_bursts_test(uhd_example_test_case):
    """ Run test_messages. """
    tests = {
        'default': {
            'nsamps': 10000,
            'rate': 5e6,
            'channels': '0',
        },
    }

    def setup_example(self):
        """
        Set args.
        """
        self.test_params = uhd_tx_bursts_test.tests

    def run_test(self, test_name, test_args):
        """ Run the app and scrape for the failure messages. """
        self.log.info('Running test {name}, Channel = {channel}, Sample Rate = {rate}'.format(
            name=test_name, channel=test_args.get('channel'), rate=test_args.get('rate'),
        ))
        # Run example:
        args = [
            self.create_addr_args_str(),
            '--nsamps', str(test_args['nsamps']),
            '--channels', str(test_args['channels']),
            '--rate', str(test_args.get('rate', 1e6)),
        ]
        if test_args.has_key('subdev'):
            args.append('--subdev')
            args.append(test_args['subdev'])
        (app, run_results) = self.run_example('tx_bursts', args)
        # Evaluate pass/fail:
        run_results['passed'] = all([
            app.returncode == 0,
            not run_results['has_S'],
        ])
        run_results['async_burst_ack_found'] = re.search('success', app.stdout) is not None
        self.report_example_results(test_name, run_results)
        return run_results

