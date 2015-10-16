#!/usr/bin/env python
#
# Copyright 2015 Ettus Research LLC
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
""" Test using benchmark_rate. """

import re
from uhd_test_base import shell_application, uhd_example_test_case

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
        self.log.info('Running test {n}, Channel = {c}, Sample Rate = {r}'.format(
            n=test_name, c=test_args.get('chan', '0'), r=test_args.get('rate', 1e6),
        ))
        args = [
            self.create_addr_args_str(),
            '--duration', str(test_args.get('duration', 1)),
            '--channels', str(test_args.get('chan', '0')),
        ]
        if 'tx' in test_args['direction']:
            args.append('--tx_rate')
            args.append(str(test_args.get('rate', 1e6)))
        if 'rx' in test_args['direction']:
            args.append('--rx_rate')
            args.append(str(test_args.get('rate')))
        (app, run_results) = self.run_example('benchmark_rate', args)
        match = re.search(r'(Num received samples):\s*(.*)', app.stdout)
        run_results['num_rx_samples'] = int(match.group(2)) if match else -1
        match = re.search(r'(Num dropped samples):\s*(.*)', app.stdout)
        run_results['num_rx_dropped'] = int(match.group(2)) if match else -1
        match = re.search(r'(Num overflows detected):\s*(.*)', app.stdout)
        run_results['num_rx_overruns'] = int(match.group(2)) if match else -1
        match = re.search(r'(Num transmitted samples):\s*(.*)', app.stdout)
        run_results['num_tx_samples'] = int(match.group(2)) if match else -1
        match = re.search(r'(Num sequence errors):\s*(.*)', app.stdout)
        run_results['num_tx_seqerrs'] = int(match.group(2)) if match else -1
        match = re.search(r'(Num underflows detected):\s*(.*)', app.stdout)
        run_results['num_tx_underruns'] = int(match.group(2)) if match else -1
        run_results['passed'] = all([
            run_results['return_code'] == 0,
            run_results['num_rx_dropped'] == 0,
            run_results['num_tx_seqerrs'] == 0,
            run_results['num_tx_underruns'] <= test_args.get('acceptable-underruns', 0),
        ])
        self.report_example_results(test_name, run_results)
        return run_results

