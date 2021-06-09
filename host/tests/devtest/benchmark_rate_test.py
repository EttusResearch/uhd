#!/usr/bin/env python3
#
# Copyright 2015-2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test using benchmark_rate. """

import re
from uhd_test_base import uhd_example_test_case

DEFAULT_O_THRESHOLD = 1000
DEFAULT_U_THRESHOLD = 1000
DEFAULT_D_THRESHOLD = 50
DEFAULT_S_THRESHOLD = 50

class uhd_benchmark_rate_test(uhd_example_test_case):
    """
    Run benchmark_rate in various configurations.
    """
    tests = {}

    def setup_example(self):
        """
        Set args.
        """
        self.test_params = uhd_benchmark_rate_test.tests

    def run_test(self, test_name, test_args):
        """
        Runs benchmark_rate with the given parameters. Parses output and writes
        results to file.
        """
        # rel_samp_err_threshold = 0.1 # 10% off is still quite generous
        samp_rate = test_args.get('rate', 1e6)
        duration = test_args.get('duration', 1)
        chan = test_args.get('chan', '0')
        n_chans = len(chan.split(","))
        expected_rx_samples = n_chans * duration * samp_rate \
                if 'rx' in test_args.get('direction', '') else 0
        expected_tx_samples = n_chans * duration * samp_rate \
                if 'tx' in test_args.get('direction', '') else 0
        self.log.info('Running test {n}, Channel = {c}, Sample Rate = {r}'.format(
            n=test_name, c=chan, r=samp_rate,
        ))
        args = [
            self.create_addr_args_str(),
            '--duration', str(duration),
            '--channels', str(chan),
            '--overrun-threshold',
            str(test_args.get('acceptable-overruns', DEFAULT_O_THRESHOLD)),
            '--underrun-threshold',
            str(test_args.get('acceptable-underruns', DEFAULT_U_THRESHOLD)),
            '--drop-threshold',
            str(test_args.get('acceptable-D', DEFAULT_D_THRESHOLD)),
            '--seq-threshold',
            str(test_args.get('acceptable-S', DEFAULT_S_THRESHOLD)),
        ]
        if 'tx' in test_args.get('direction', ''):
            args.append('--tx_rate')
            args.append(str(samp_rate))
        if 'rx' in test_args.get('direction', ''):
            args.append('--rx_rate')
            args.append(str(samp_rate))
        (app, run_results) = self.run_example('benchmark_rate', args)

        match = re.search(r'(Num received samples):\s*(.*)', app.stdout)
        run_results['num_rx_samples'] = int(match.group(2)) if match else -1
        if expected_rx_samples != 0:
            if run_results['num_rx_samples'] != -1:
                run_results['rel_rx_samples_error'] = 1.0 * abs(
                    run_results['num_rx_samples']
                    - test_args.get('rx_buffer', 0)
                    - expected_rx_samples
                ) / expected_rx_samples
            else:
                run_results['rel_rx_samples_error'] = 100
        else:
            run_results['rel_rx_samples_error'] = 0 \
                    if run_results['num_rx_samples'] == 0 else 1
        match = re.search(r'(Num dropped samples):\s*(.*)', app.stdout)
        run_results['num_rx_dropped'] = int(match.group(2)) if match else -1
        match = re.search(r'(Num overruns detected):\s*(.*)', app.stdout)
        run_results['num_rx_overruns'] = int(match.group(2)) if match else -1
        match = re.search(r'(Num transmitted samples):\s*(.*)', app.stdout)
        run_results['num_tx_samples'] = int(match.group(2)) if match else -1
        if expected_tx_samples != 0:
            if run_results['num_tx_samples'] != -1:
                run_results['rel_tx_samples_error'] = 1.0 * abs(
                    run_results['num_tx_samples']
                    - test_args.get('tx_buffer', 0)
                    - expected_tx_samples
                ) / expected_tx_samples
            else:
                run_results['rel_tx_samples_error'] = 100
        else:
            run_results['rel_tx_samples_error'] = 0 \
                    if run_results['num_tx_samples'] == 0 else 1
        match = re.search(r'(Num sequence errors \(Tx\)):\s*(.*)', app.stdout)
        run_results['num_tx_seqerrs'] = int(match.group(2)) if match else -1
        match = re.search(r'(Num underruns detected):\s*(.*)', app.stdout)
        run_results['num_tx_underruns'] = int(match.group(2)) if match else -1
        match = re.search(r'(Num timeouts \(Rx\)):\s*(.*)', app.stdout)
        run_results['num_timeouts_rx'] = int(match.group(2)) if match else -1
        run_results['passed'] = all([
            run_results['return_code'] == 0,
            run_results['num_rx_dropped'] == 0,
            run_results['num_tx_seqerrs'] == 0,
            run_results['num_tx_underruns'] <= test_args.get('acceptable-underruns', 0),
            ((not 'rx' in test_args.get('direction', '')) or (run_results['num_rx_samples'] > 0)),
            ((not 'tx' in test_args.get('direction', '')) or (run_results['num_tx_samples'] > 0)),
            run_results['num_timeouts_rx'] == 0,
            # run_results['rel_rx_samples_error'] < rel_samp_err_threshold,
            # run_results['rel_tx_samples_error'] < rel_samp_err_threshold,
        ])
        self.report_example_results(test_name, run_results)
        return run_results
