#!/usr/bin/env python3
#
# Copyright 2015-2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Devtest: Base module. Provides classes for running devtest tests.
"""

import os
import sys
import unittest
import re
import time
import logging
import importlib
from subprocess import Popen, PIPE
# For what we're doing here, ruamel.yaml and yaml are compatible, and we'll use
# whatever we can find
try:
    from ruamel import yaml
except:
    import yaml
from usrp_probe import get_usrp_list

#--------------------------------------------------------------------------
# Helpers
#--------------------------------------------------------------------------
def filter_warnings(errstr):
    """
    Searches errstr for UHD warnings, removes them, and puts them into a
    separate string.
    Returns (errstr, warnstr), where errstr no longer has warnings. """
    warn_re = re.compile("UHD Warning:\n(?:    .*\n)+")
    warnstr = "\n".join(warn_re.findall(errstr)).strip()
    errstr = warn_re.sub('', errstr).strip()
    return (errstr, warnstr)

def filter_stderr(stderr, run_results=None):
    """
    Filters the output to stderr. run_results[] is a dictionary.
    This function will:
    - Remove warnings and put them in run_results['warnings']
    - Put the filtered error string into run_results['errors'] and returns the dictionary
    """
    run_results = run_results or {}
    errstr, run_results['warnings'] = filter_warnings(stderr)
    # Scan for underruns and sequence errors / dropped packets  not detected in the counter
    errstr = re.sub("\n\n+", "\n", errstr)
    run_results['errors'] = errstr.strip()
    return run_results

#--------------------------------------------------------------------------
# Application
#--------------------------------------------------------------------------
class shell_application:
    """
    Wrapper for applications that are in $PATH.
    Note: The CMake infrastructure makes sure all examples and utils are in $PATH.
    """
    def __init__(self, name):
        self.name = name
        self.stdout = ''
        self.stderr = ''
        self.returncode = None
        self.exec_time = None

    def run(self, args=None):
        """Test executor."""
        args = args or []
        cmd_line = [self.name]
        cmd_line.extend(args)
        start_time = time.time()
        env = os.environ
        env["UHD_LOG_FASTPATH_DISABLE"] = "1"
        try:
            proc = Popen(
                cmd_line,
                stdout=PIPE,
                stderr=PIPE,
                close_fds=True,
                env=env,
                universal_newlines=True
            )
            self.stdout, self.stderr = proc.communicate()
            self.returncode = proc.returncode
            self.exec_time = time.time() - start_time
        except OSError as ex:
            raise RuntimeError("Failed to execute command: `{}'\n{}"
                               .format(cmd_line, str(ex)))


#--------------------------------------------------------------------------
# Test case base
#--------------------------------------------------------------------------
class uhd_test_case(unittest.TestCase):
    """
    Base class for UHD test cases.
    """
    test_name = '--TEST--'

    def set_up(self):
        """
        Override this to add own setup code per test.
        """

    def setUp(self):
        self.name = self.__class__.__name__
        self.test_id = self.id().split('.')[-1]
        self.results = {}
        self.results_file = os.getenv('_UHD_TEST_RESULTSFILE', "")
        if self.results_file and os.path.isfile(self.results_file):
            with open(self.results_file) as res_file:
                self.results = yaml.safe_load(res_file.read()) or {}
        self.args_str = os.getenv('_UHD_TEST_ARGS_STR', "")
        self.usrp_info = get_usrp_list(self.args_str)[0]
        if self.usrp_info['serial'] not in self.results:
            self.results[self.usrp_info['serial']] = {}
        if self.name not in self.results[self.usrp_info['serial']]:
            self.results[self.usrp_info['serial']][self.name] = {}
        self.setup_logger()
        self.set_up()

    def setup_logger(self):
        " Add logging infrastructure "
        self.log = logging.getLogger("devtest.{name}".format(name=self.name))
        self.log_file = os.getenv('_UHD_TEST_LOGFILE', "devtest.log")
        #self.log_level = int(os.getenv('_UHD_TEST_LOG_LEVEL', logging.DEBUG))
        #self.print_level = int(os.getenv('_UHD_TEST_PRINT_LEVEL', logging.WARNING))
        self.log_level = logging.DEBUG
        self.print_level = logging.WARNING
        file_handler = logging.FileHandler(self.log_file)
        file_handler.setLevel(self.log_level)
        console_handler = logging.StreamHandler()
        console_handler.setLevel(self.print_level)
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        file_handler.setFormatter(formatter)
        console_handler.setFormatter(formatter)
        self.log.setLevel(logging.DEBUG)
        self.log.addHandler(file_handler)
        self.log.addHandler(console_handler)
        self.log.info("Starting test with device: %s", str(self.args_str))

    def tear_down(self):
        """Nothing to do."""

    def tearDown(self):
        self.tear_down()
        if self.results_file:
            with open(self.results_file, 'w') as res_file:
                res_file.write(yaml.dump(self.results, default_flow_style=False))
        time.sleep(15)

    def report_result(self, testname, key, value):
        """ Store a result as a key/value pair.
        After completion, all results for one test are written to the results file.
        """
        if not testname in self.results[self.usrp_info['serial']][self.name]:
            self.results[self.usrp_info['serial']][self.name][testname] = {}
        self.results[self.usrp_info['serial']][self.name][testname][key] = value

    def create_addr_args_str(self, argname="args"):
        """ Returns an args string, usually '--args "type=XXX,serial=YYY" """
        if not self.args_str:
            return ''
        return '--{}={}'.format(argname, self.args_str)

class uhd_example_test_case(uhd_test_case):
    """
    A test case that runs an example.
    """

    def setup_example(self):
        """
        Override this to add specific setup code.
        """

    def set_up(self):
        """Called by the unit testing framework on tests. """
        self.setup_example()

    def run_test(self, test_name, test_args):
        """
        Override this to run the actual example.

        Needs to return either a boolean or a dict with key 'passed' to determine
        pass/fail.
        """
        raise NotImplementedError

    def run_example(self, example, args):
        """
        Run `example' (which has to be a UHD example or utility) with `args'.
        Return results and the app object.

        Note: UHD_LOG_FASTPATH_DISABLE will be set to 1.
        """
        self.log.info("Running example: `%s %s'", example, " ".join(args))
        app = shell_application(example)
        app.run(args)
        run_results = {
            'return_code': app.returncode,
            'passed': False,
        }
        run_results = filter_stderr(app.stderr, run_results)
        self.log.info('STDERR Output:')
        self.log.info(str(app.stderr))
        return (app, run_results)


    def report_example_results(self, test_name, run_results):
        """
        Helper function for report_result() when running examples.
        """
        for key in sorted(run_results):
            self.log.info('%s = %s', str(key), str(run_results[key]))
            self.report_result(
                test_name,
                key, run_results[key]
            )
        if 'passed' in run_results:
            self.report_result(
                test_name,
                'status',
                'Passed' if run_results['passed'] else 'Failed',
            )
        if 'errors' in run_results:
            self.report_result(
                test_name,
                'errors',
                'Yes' if run_results['errors'] else 'No',
            )

    def test_all(self):
        """
        Hook for test runner. Needs to be a class method that starts with 'test'.
        Calls run_test().
        """
        test_params = getattr(self, 'test_params', {})
        for test_name, test_args in test_params.items():
            time.sleep(15) # Wait for X300 devices to reclaim them
            if not 'products' in test_args \
                    or (self.usrp_info['product'] in test_args.get('products', [])):
                run_results = self.run_test(test_name, test_args)
                passed = bool(run_results)
                errors = ''
                if isinstance(run_results, dict):
                    passed = run_results['passed']
                    errors = run_results.pop("errors", None)
                    if not passed:
                        print("Error log:", file=sys.stderr)
                        print(errors)
                self.assertTrue(
                    passed,
                    msg="Errors occurred during test `{t}'. "
                        "Check log file for details.\n"
                        "Run results:\n{r}".format(
                            t=test_name,
                            r=yaml.dump(run_results, default_flow_style=False)
                        )
                )

class UHDPythonTestCase(uhd_test_case):
    """
    Helper class for Python test cases. These require the uhd module, but that's
    not always available. We thus test for its existence.
    """

    def run_test(self, test_name, test_args):
        """
        Override this to run the actual example.

        Needs to return either a boolean or a dict with key 'passed' to determine
        pass/fail.
        """
        raise NotImplementedError

    def test_all(self):
        """
        Hook for test runner. Needs to be a class method that starts with 'test'.
        Calls run_test().
        """
        try:
            self.uhd = importlib.import_module('uhd')
        except ImportError:
            print("UHD module not found -- checking for Python API")
            config_info_app = shell_application('uhd_config_info')
            config_info_app.run(['--enabled-components'])
            if "Python API" in config_info_app.stdout:
                raise RuntimeError("Python API enabled, but cannot load uhd module!")
            self.log.info("Skipping test, Python API not installed.")
            self.report_result("python_api_tester", 'status', 'Skipped')
            return
        # Now: The actual test
        test_params = getattr(self, 'test_params', {'default': {},})
        for test_name, test_args in test_params.items():
            time.sleep(15) # Wait for X300 devices to reclaim them
            if 'products' not in test_args \
                    or (self.usrp_info['product'] in test_args.get('products', [])):
                run_results = self.run_test(test_name, test_args)
                passed = bool(run_results)
                if isinstance(run_results, dict):
                    passed = run_results['passed']
                    errors = run_results.pop("errors", None)
                    if not passed:
                        print("Error log:", file=sys.stderr)
                        print(errors)
                self.assertTrue(
                    passed,
                    msg="Errors occurred during test `{t}'. "
                        "Check log file for details.\n"
                        "Run results:\n{r}".format(
                            t=test_name,
                            r=yaml.dump(run_results, default_flow_style=False)
                        )
                )
