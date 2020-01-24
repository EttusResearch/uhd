#!/usr/bin/env python
#
# Copyright 2016 Ettus Research LLC
#

import argparse
import os
import subprocess
import logging
import re
import json

_LIB_DIR = os.path.join("lib")
_RFNOC_DIR = os.path.join("lib", "rfnoc")
_SIM_DIR = os.path.join("lib", "sim")
_BASE_DIR = os.path.dirname(os.path.realpath(__file__))

_SEARCH_BASE = [_RFNOC_DIR, _SIM_DIR]
_LOG = logging.getLogger(os.path.basename(__file__))
_LOG.setLevel(logging.INFO)
_STDOUT = logging.StreamHandler()
_LOG.addHandler(_STDOUT)
_FORMATTER = logging.Formatter('[%(name)s] - %(levelname)s - %(message)s')
_STDOUT.setFormatter(_FORMATTER)


def match_file(expr, path):
    """ Match regex expr on all lines of path and return list of matches """
    matches = []
    with open(path, 'rb') as my_file:
        for line in my_file:
            match = expr.match(line)
            if match:
                matches.append(match)
    return matches


def create_index(paths):
    """ Create an index of modules in .v/.vhd and dump it to modules.json """
    hdl_expressions = {
        re.compile(r".*\.v$"):
        re.compile(b"module (?P<mod_name>[\\w]+) *$", re.IGNORECASE),
        re.compile(r".*\.vhd$"):
        re.compile(b"entity (?P<mod_name>[\\w]+) is *$", re.IGNORECASE)
    }
    ignore_dirs = ["build-ip", "sim"]

    modules = {}
    for path in paths:
        for root, dirs, files in os.walk(os.path.join(_BASE_DIR, path)):
            ignore = [my_dir in dirs for my_dir in ignore_dirs]
            if any(ignore):
                for index, ignore_me in enumerate(ignore):
                    if not ignore_me:
                        continue
                    dirs.pop(dirs.index(ignore_dirs[index]))
            for my_file in files:
                matches = []
                for key, value in hdl_expressions.items():
                    if key.match(my_file):
                        matches = match_file(value,
                                             os.path.join(root, my_file))
                        break

                for match in matches:
                    if match.group("mod_name") in modules:
                        _LOG.error("%s is already in modules",
                                   match.group("mod_name"))
                        _LOG.error("Old Path: %s",
                                   modules[match.group("mod_name")])
                        _LOG.error("New Path: %s", os.path.join(root, my_file))
                    else:
                        modules.update({
                            match.group("mod_name").decode('utf-8'):
                            os.path.join(root, my_file)
                        })
    with open("modules.json", "w") as my_file:
        json.dump(
            modules, my_file, sort_keys=True, indent=4, separators=(',', ': '))


def call_xsim(path):
    """ Call make xsim with default environment at path """
    os.chdir(os.path.join(_BASE_DIR, path))
    env = os.environ
    env["REPO_BASE_PATH"] = _BASE_DIR
    env["DISPLAY_NAME"] = "USRP-XSIM"
    env["VIVADO_VER"] = "2017.4"
    env["PRODUCT_ID_MAP"] = "foo/foo/bar/bar"
    setup_env = os.path.join(_BASE_DIR, "tools", "scripts", "setupenv_base.sh")
    result = subprocess.Popen(
        ". {setup}; make xsim".format(setup=setup_env), env=env,
        shell=True).wait()
    return result


def find_xsims():
    """ Find testbenches in lib/sim (dirs with Makefile) """
    sims = {}
    for basedir in _SEARCH_BASE:
        for root, _, files in os.walk(os.path.join(_BASE_DIR, basedir)):
            if "Makefile" in files:
                sims.update({os.path.basename(root): root})
    return sims


def run_xsim(args):
    """ Run xsim for all specified modules """
    sims = find_xsims()
    result_all = 0
    if not isinstance(args.target, list):
        args.target = [args.target]
    if "cleanall" in args.target:
        env = os.environ
        env["REPO_BASE_PATH"] = _BASE_DIR
        for name, path in sims.iteritems():
            _LOG.info("Cleaning %s", name)
            os.chdir(os.path.join(_BASE_DIR, path))
            subprocess.Popen("make cleanall", env=env, shell=True).wait()
    elif "all" in args.target:
        for name, path in sims.iteritems():
            _LOG.info("Running %s xsim", name)
            result = call_xsim(path)
            if result:
                result_all = result
    else:
        for target in args.target:
            _LOG.info("Running %s xsim", target)
            result = call_xsim(sims[target])
            if result:
                result_all = result
    return result_all


def parse_args():
    """ Parse cmdline arguments"""
    test_benches = find_xsims()
    parser = argparse.ArgumentParser()
    subparser = parser.add_subparsers(dest="command", metavar="")
    xsim_parser = subparser.add_parser(
        "xsim", help="Run available testbenches")
    xsim_parser.add_argument(
        "target",
        nargs="+",
        choices=list(test_benches.keys()) + ["all", "cleanall"],
        help="Space separated simulation target(s) or all. Available targets: "
        + ", ".join(list(test_benches.keys()) + ["all", "cleanall"]),
        metavar="")
    index_parser = subparser.add_parser(
        "index", help="Index available HDL modules")
    index_parser.add_argument(
        "create",
        help="Create a modules.json of available HDL modules in lib/")
    return parser.parse_args()


def main():
    """Main logic"""
    args = parse_args()
    result = 0
    if args.command == "xsim":
        result = run_xsim(args)
    elif args.command == "index":
        create_index([_LIB_DIR])

    return result


if __name__ == "__main__":
    exit(not main())
