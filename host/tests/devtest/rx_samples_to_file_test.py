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
""" Test the rx_samples_to_file example. """

from uhd_test_base import uhd_example_test_case

class rx_samples_to_file_test(uhd_example_test_case):
    """
    Run rx_samples_to_file and check output.
    """
    tests = {
        'default': {
            'duration': 1,
            'rate': 5e6,
        },
    }

    def setup_example(self):
        """
        Set args.
        """
        self.test_params = rx_samples_to_file_test.tests

    def run_test(self, test_name, test_args):
        """
        Test launcher. Runs the example.
        """
        self.log.info('Running test {n}, Subdev = {subdev}, Sample Rate = {rate}'.format(
            n=test_name, subdev=test_args.get('subdev'), rate=test_args.get('rate'),
        ))
        # Run example:
        args = [
            self.create_addr_args_str(),
            '--null',
            '--stats',
            '--duration', str(test_args['duration']),
            '--rate', str(test_args.get('rate', 1e6)),
            '--wirefmt', test_args.get('wirefmt', 'sc16'),
        ]
        if test_args.has_key('subdev'):
            args.append('--subdev')
            args.append(test_args['subdev'])
        (app, run_results) = self.run_example('rx_samples_to_file', args)
        # Evaluate pass/fail:
        run_results['passed'] = all([
            not run_results['has_D'],
            not run_results['has_S'],
            run_results['return_code'] == 0,
        ])
        self.report_example_results(test_name, run_results)
        return run_results

