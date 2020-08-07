#!/usr/bin/env python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test Python API """

import os
from uhd_test_base import shell_application
from uhd_test_base import uhd_test_case
try:
    import uhd
except ImportError:
    uhd = None

# For what we're doing here, ruamel.yaml and yaml are compatible, and we'll use
# whatever we can find
try:
    from ruamel import yaml
except:
    import yaml


class uhd_python_api_test(uhd_test_case):
    """ Run multi_usrp_test """
    def test_api(self):
        """
        Run test and report results.
        """
        if uhd is None:
            print("UHD module not found -- checking for Python API")
            config_info_app = shell_application('uhd_config_info')
            config_info_app.run(['--enabled-components'])
            if "Python API" in config_info_app.stdout:
                raise RuntimeError("Python API enabled, but cannot load uhd module!")
            print("Skipping test, Python API not installed.")
            self.report_result("python_api_tester", 'status', 'Skipped')
            return
        devtest_src_dir = os.getenv('_UHD_DEVTEST_SRC_DIR', '')
        multi_usrp_test_path = \
            os.path.join(devtest_src_dir, 'multi_usrp_test.py')
        args = [
            self.create_addr_args_str(),
        ]
        app = None
        if os.name == 'nt':
            args.insert(0, multi_usrp_test_path)
            app = shell_application('python')
        else:
            app = shell_application(multi_usrp_test_path)
        app.run(args)
        run_results = {
            'return_code': app.returncode,
            'passed': False
        }
        run_results['passed'] = all([
            app.returncode == 0,
        ])
        self.log.info('STDERR Output:')
        self.log.info(str(app.stderr))
        for key in sorted(run_results):
            self.log.info('%s = %s', str(key), str(run_results[key]))
            self.report_result(
                "python_api_tester",
                key, run_results[key]
            )
        if 'passed' in run_results:
            self.report_result(
                "python_api_tester",
                'status',
                'Passed' if run_results['passed'] else 'Failed',
            )
        self.assertTrue(
            run_results['passed'],
            msg="Errors occurred during test python_api_test. "
                "Check log file for details.\n"
                "Run results:\n{r}".format(
                    r=yaml.dump(run_results, default_flow_style=False)
                )
        )
