#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test for gpio. """

import re
from uhd_test_base import uhd_example_test_case, uhd_test_case, UHDPythonTestCase
import random

class gpio_test(uhd_example_test_case):
    """ Run gpio. """
    tests = {'default': {
        'addl_args': [],
    },}

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
        args += test_args['addl_args']
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


class gpio_x410_set_get_source_test(UHDPythonTestCase):
    test_params = {
        "possible_sources": [],
        "num_pins": 0,
    }

    def test_all(self):
        import uhd
        usrp = uhd.usrp.MultiUSRP(self.args_str)

        POSSIBLE_SOURCES = self.test_params["possible_sources"]

        # Assemble two lists which have at least one of each of the 7 sources,
        # with the remaining 12-7=5 entries containing random sources.
        sources_0 = [x for x in POSSIBLE_SOURCES]
        sources_1 = [x for x in POSSIBLE_SOURCES]
        for _ in range(self.test_params["num_pins"] - len(POSSIBLE_SOURCES)):
            sources_0.append(random.choice(POSSIBLE_SOURCES))
            sources_1.append(random.choice(POSSIBLE_SOURCES))
        random.shuffle(sources_0)
        random.shuffle(sources_1)

        usrp.set_gpio_src("GPIO0", sources_0)
        usrp.set_gpio_src("GPIO1", sources_1)

        assert sources_0 == list(usrp.get_gpio_src("GPIO0"))
        assert sources_1 == list(usrp.get_gpio_src("GPIO1"))


class x410_gpio_power_test(UHDPythonTestCase):
    """ Run gpio_power_test """
    test_name = "x410_gpio_power_test"

    def run_test(self, test_name, test_args):
        """
        Run test and report results.
        """
        import uhd
        usrp = uhd.usrp.MultiUSRP(self.args_str)

        gpio_power = usrp.get_mb_controller().get_gpio_power()
        assert usrp.get_mb_controller().get_gpio_power().get_supported_voltages("GPIO0") == ['OFF', '1V8', '2V5', '3V3']

        for port in ["GPIO0", "GPIO1"]:
            assert gpio_power.get_port_voltage(port) == "3V3"
            for voltage in gpio_power.get_supported_voltages(port):
                gpio_power.set_port_voltage(port, voltage)
                assert gpio_power.get_port_voltage(port) == voltage

        return {"passed": True}
