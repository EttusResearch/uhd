#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test for gpio. """

import re
from uhd_test_base import uhd_example_test_case, uhd_test_case, UHDPythonTestCase

class gpio_test(uhd_example_test_case):
    """ Run gpio. """
    tests = {'default': {},}

    def setup_example(self):
        """
        Set args.
        """
        self.test_params = gpio_test.tests

    def run_test(self, test_name, test_args):
        """ Run the app and scrape for the success message. """
        self.log.info('Running test {n}'.format(n=test_name,))
        # Run example:
        args = [
            self.create_addr_args_str(),
        ]
        (app, run_results) = self.run_example('gpio', args)
        # Evaluate pass/fail:
        run_results['passed'] = all([
            app.returncode == 0,
            re.search('All tests passed!', app.stdout) is not None,
        ])
        if not run_results['passed']:
            print(app.stdout)
            print(app.stderr)
        self.report_example_results(test_name, run_results)
        return run_results


class gpio_atr_readback_test(UHDPythonTestCase):
    test_params = []

    def test_all(self):
        import uhd
        usrp = uhd.usrp.MultiUSRP(self.args_str)

        for gpio, source in self.test_params:
            usrp.set_gpio_src("GPIO0", [source] * 12)

            usrp.set_gpio_attr(gpio, "ATR_0X", 0xABC, 0xFFF)
            usrp.set_gpio_attr(gpio, "ATR_RX", 0xCAB, 0xFFF)
            usrp.set_gpio_attr(gpio, "ATR_TX", 0xABA, 0xFFF)
            usrp.set_gpio_attr(gpio, "ATR_XX", 0xBAB, 0xFFF)

            assert usrp.get_gpio_attr(gpio, "ATR_0X") == 0xABC
            assert usrp.get_gpio_attr(gpio, "ATR_RX") == 0xCAB
            assert usrp.get_gpio_attr(gpio, "ATR_TX") == 0xABA
            assert usrp.get_gpio_attr(gpio, "ATR_XX") == 0xBAB
