#!/usr/bin/env python
#
# Copyright 2015-2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Device test runner.
"""

from __future__ import print_function
import os
import sys
import subprocess
import argparse
import logging
from usrp_probe import get_usrp_list

def setup_parser():
    """ Set up argparser """
    parser = argparse.ArgumentParser(description="Test utility for UHD/USRP.")
    parser.add_argument('--devtest-pattern', '-p', default='*', help='e.g. b2xx')
    parser.add_argument('--device-filter', '-f', default=None, required=True, help='b200, x300, ...')
    parser.add_argument('--log-dir', '-l', default='.')
    parser.add_argument('--src-dir', default='.', help='Directory where the test sources are stored')
    parser.add_argument('--build-dir', default=None, help='Build dir (where examples/ and utils/ are)')
    parser.add_argument('--build-type', default='Release')
    return parser

def setup_env(args):
    " Add build dir into lib + exe paths, depending on OS "
    def setup_env_win(env, build_dir, build_type):
        " Add build dir into paths (Windows)"
        env['PATH'] = "{build_dir}/lib/{build_type};" + \
                      "{build_dir}/examples/{build_type};" + \
                      "{build_dir}/utils/{build_type};{path}".format(
            build_dir=build_dir, build_type=build_type, path=env.get('PATH', '')
        )
        env['LIBPATH'] = "{build_dir}/lib/{build_type};{path}".format(
            build_dir=build_dir, build_type=build_type, path=env.get('LIBPATH', '')
        )
        env['LIB'] = "{build_dir}/lib/{build_type};{path}".format(
            build_dir=build_dir, build_type=build_type, path=env.get('LIB', '')
        )
        return env
    def setup_env_unix(env, build_dir):
        " Add build dir into paths (Unices)"
        env['PATH'] = "{build_dir}/examples:{build_dir}/utils:{path}".format(
            build_dir=build_dir, path=env.get('PATH', '')
        )
        env['LD_LIBRARY_PATH'] = "{build_dir}/lib:{path}".format(
            build_dir=build_dir, path=env.get('LD_LIBRARY_PATH', '')
        )
        return env
    def setup_env_osx(env, build_dir):
        " Add build dir into paths (OS X)"
        env['PATH'] = "{build_dir}/examples:{build_dir}/utils:{path}".format(
                build_dir=build_dir, path=env.get('PATH', '')
        )
        env['DYLD_LIBRARY_PATH'] = "{build_dir}/lib:{path}".format(
                build_dir=build_dir, path=env.get('DYLD_LIBRARY_PATH', '')
        )
        return env
    ### Go
    env = os.environ
    if sys.platform.startswith('linux'):
        env = setup_env_unix(env, args.build_dir)
    elif sys.platform.startswith('win'):
        env = setup_env_win(env, args.build_dir, args.build_type)
    elif sys.platform.startswith('darwin'):
        env = setup_env_osx(env, args.build_dir)
    else:
        print("Devtest not supported on this platform ({0}).".format(sys.platform))
        exit(1)
    return env

def main():
    " Go, go, go! "
    args = setup_parser().parse_args()
    env = setup_env(args)
    devtest_pattern = "devtest_{p}.py".format(p=args.devtest_pattern)
    uhd_args_list = get_usrp_list("type=" + args.device_filter, env)
    if len(uhd_args_list) == 0:
        print("No devices found. Exiting.")
        exit(1)
    tests_passed = True
    for uhd_idx, uhd_info in enumerate(uhd_args_list):
        print('--- Running all tests for device {dev} ({prod}, Serial: {ser}).'.format(
            dev=uhd_idx,
            prod=uhd_info.get('product', 'USRP'),
            ser=uhd_info.get('serial')
        ))
        print('--- This will take some time. Better grab a cup of tea.')
        sys.stdout.flush()
        args_str = uhd_info['args']
        env['_UHD_TEST_ARGS_STR'] = args_str
        logfile_name = "log{}.log".format(
            args_str.replace('type=', '_').replace('serial=', '_').replace(',', '')
        )
        resultsfile_name = "results{}.log".format(
            args_str.replace('type=', '_').replace('serial=', '_').replace(',', '')
        )
        env['_UHD_TEST_LOGFILE'] = os.path.join(args.log_dir, logfile_name)
        env['_UHD_TEST_RESULTSFILE'] = os.path.join(args.log_dir, resultsfile_name)
        env['_UHD_TEST_LOG_LEVEL'] = str(logging.INFO)
        env['_UHD_TEST_PRINT_LEVEL'] = str(logging.WARNING)
        proc = subprocess.Popen(
            [
                "python", "-m", "unittest", "discover", "-v",
                "-s", args.src_dir,
                "-p", devtest_pattern,
            ],
            env=env,
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        print(proc.communicate()[0])
        sys.stdout.flush()
        if proc.returncode != 0:
            tests_passed = False
    print('--- Done testing all attached devices.')
    return tests_passed

if __name__ == "__main__":
    exit(not main())

