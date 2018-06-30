#!/usr/bin/env python
#
# Copyright 2015-2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import os
import sys
import yaml
import unittest
import re
import time
import logging
from subprocess import Popen, PIPE, STDOUT
from usrp_probe import get_usrp_list

#--------------------------------------------------------------------------
# Application
#--------------------------------------------------------------------------
class shell_application(object):
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

    def run(self, args = []):
        cmd_line = [self.name]
        cmd_line.extend(args)
        start_time = time.time()
        p = Popen(cmd_line, stdout=PIPE, stderr=PIPE, close_fds=True)
        self.stdout, self.stderr = p.communicate()
        self.returncode = p.returncode
        self.exec_time = time.time() - start_time

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
        pass

    def setUp(self):
        self.name = self.__class__.__name__
        self.test_id = self.id().split('.')[-1]
        self.results = {}
        self.results_file = os.getenv('_UHD_TEST_RESULTSFILE', "")
        if self.results_file and os.path.isfile(self.results_file):
            self.results = yaml.safe_load(open(self.results_file).read()) or {}
        self.args_str = os.getenv('_UHD_TEST_ARGS_STR', "")
        self.usrp_info = get_usrp_list(self.args_str)[0]
        if not self.results.has_key(self.usrp_info['serial']):
            self.results[self.usrp_info['serial']] = {}
        if not self.results[self.usrp_info['serial']].has_key(self.name):
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
        self.log.info("Starting test with device: {dev}".format(dev=self.args_str))

    def tear_down(self):
        pass

    def tearDown(self):
        self.tear_down()
        if self.results_file:
            open(self.results_file, 'w').write(yaml.dump(self.results, default_flow_style=False))
        time.sleep(15)
    def report_result(self, testname, key, value):
        """ Store a result as a key/value pair.
        After completion, all results for one test are written to the results file.
        """
        if not self.results[self.usrp_info['serial']][self.name].has_key(testname):
            self.results[self.usrp_info['serial']][self.name][testname] = {}
        self.results[self.usrp_info['serial']][self.name][testname][key] = value

    def create_addr_args_str(self, argname="args"):
        """ Returns an args string, usually '--args "type=XXX,serial=YYY" """
        if len(self.args_str) == 0:
            return ''
        return '--{}={}'.format(argname, self.args_str)

    def filter_warnings(self, errstr):
        """ Searches errstr for UHD warnings, removes them, and puts them into a separate string.
        Returns (errstr, warnstr), where errstr no longer has warning. """
        warn_re = re.compile("UHD Warning:\n(?:    .*\n)+")
        warnstr = "\n".join(warn_re.findall(errstr)).strip()
        errstr = warn_re.sub('', errstr).strip()
        return (errstr, warnstr)

    def filter_stderr(self, stderr, run_results={}):
        """ Filters the output to stderr. run_results[] is a dictionary.
        This function will:
        - Remove UUUUU... strings, since they are generally not a problem.
        - Remove all DDDD and SSSS strings, and add run_results['has_S'] = True
          and run_results['has_D'] = True.
        - Remove warnings and put them in run_results['warnings']
        - Put the filtered error string into run_results['errors'] and returns the dictionary
        """
        errstr, run_results['warnings'] = self.filter_warnings(stderr)
        # Scan for underruns and sequence errors / dropped packets  not detected in the counter
        errstr = re.sub('UU+', '', errstr)
        (errstr, n_subs) = re.subn('SS+', '', errstr)
        if n_subs:
            run_results['has_S'] = True
        (errstr, n_subs) = re.subn('DD+', '', errstr)
        if n_subs:
            run_results['has_D'] = True
        errstr = re.sub("\n\n+", "\n", errstr)
        run_results['errors'] = errstr.strip()
        return run_results

class uhd_example_test_case(uhd_test_case):
    """
    A test case that runs an example.
    """

    def setup_example(self):
        """
        Override this to add specific setup code.
        """
        pass

    def set_up(self):
        """
        """
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
        """
        self.log.info("Running example: `{example} {args}'".format(example=example, args=" ".join(args)))
        app = shell_application(example)
        app.run(args)
        run_results = {
            'return_code': app.returncode,
            'passed': False,
            'has_D': False,
            'has_S': False,
        }
        run_results = self.filter_stderr(app.stderr, run_results)
        self.log.info('STDERR Output:')
        self.log.info(str(app.stderr))
        return (app, run_results)


    def report_example_results(self, test_name, run_results):
        for key in sorted(run_results):
            self.log.info('{key} = {val}'.format(key=key, val=run_results[key]))
            self.report_result(
                test_name,
                key, run_results[key]
            )
        if run_results.has_key('passed'):
            self.report_result(
                test_name,
                'status',
                'Passed' if run_results['passed'] else 'Failed',
            )
        if run_results.has_key('errors'):
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
        for test_name, test_args in self.test_params.iteritems():
            time.sleep(15) # Wait for X300 devices to reclaim them
            if not test_args.has_key('products') or (self.usrp_info['product'] in test_args.get('products', [])):
                run_results = self.run_test(test_name, test_args)
                passed = bool(run_results)
                if isinstance(run_results, dict):
                    passed = run_results['passed']
                self.assertTrue(
                    passed,
                    msg="Errors occurred during test `{t}'. Check log file for details.\nRun results:\n{r}".format(
                        t=test_name, r=yaml.dump(run_results, default_flow_style=False)
                    )
                )

