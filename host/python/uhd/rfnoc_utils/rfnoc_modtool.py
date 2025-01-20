#!/usr/bin/env python3
"""RFNoC Modtool: The tool to create and manipulate RFNoC OOT modules."""
#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import argparse
import glob
import os
import sys

from ruamel.yaml import YAML
from uhd import get_pkg_data_path

from .log import init_logging
from .step_executor import StepExecutor
from .utils import resolve


def get_command_repo_dir():
    """Return the path to the command repository directory (where the command YAMLS are stored)."""
    this_dir = os.path.dirname(os.path.realpath(__file__))
    return os.path.join(this_dir, "modtool_commands")


def collect_commands():
    """Load all available commands.

    The return value is a dictionary where the keys are the command names and
    the values are raw contents of the YAML as Python structures.
    """
    yamls = glob.glob(os.path.join(get_command_repo_dir(), "*.yml"))
    yaml = YAML()
    commands = {}
    for y in yamls:
        cmd_name = os.path.basename(y).replace(".yml", "")
        with open(y, "r", encoding="utf-8") as f:
            commands[cmd_name] = yaml.load(f)
    return commands


def parse_args(cmds):
    """Parse args.

    This will dynamically populate main- and subparsers from the YAML configs
    given in cmds.
    """
    arg_parser = argparse.ArgumentParser(description=__doc__)
    arg_parser.add_argument(
        "-C", "--directory", help="Change to this directory before running the command"
    )
    arg_parser.add_argument(
        "-l", "--log-level", default="INFO", help="Set the log level (default: INFO)"
    )
    subparsers = arg_parser.add_subparsers(dest="command")
    parser_list = []
    for cmd in cmds:
        new_parser = subparsers.add_parser(cmd, help=cmds[cmd]["help"])
        for arg_name, arg_info in cmds[cmd]["args"].items():
            name_or_flags = (
                f"{arg_name}" if "name_or_flags" not in arg_info else arg_info.pop("name_or_flags")
            )
            if not isinstance(name_or_flags, list):
                name_or_flags = [name_or_flags]
            # We don't want to use eval(), but for most types we need to do
            # just that, so manually map
            if "type" in arg_info:
                try:
                    arg_info["type"] = {
                        "str": str,
                        "int": int,
                        "float": float,
                    }[arg_info["type"]]
                except KeyError:
                    pass
            new_parser.add_argument(*name_or_flags, **arg_info)
        parser_list.append(new_parser)
    return arg_parser.parse_args()


def get_global_vars(pkg_data_dir):
    """Create a dictionary with global variables for the key resolution."""
    return {
        "RFNOC_PKG_DIR": pkg_data_dir,
        "CWD": os.getcwd(),
    }


def resolve_vars(cmd, global_vars, args):
    """Resolve all variables in the command."""
    for var_name, var_val in cmd.get("variables", {}).items():
        cmd["variables"][var_name] = resolve(
            var_val, args=args, **global_vars, **cmd.get("variables", {})
        )
    return cmd


def check_valid_oot_dir(oot_dir):
    """Check if the given directory contains a valid OOT module."""
    return os.path.isfile(os.path.join(oot_dir, "CMakeLists.txt")) and os.path.isdir(
        os.path.join(oot_dir, "rfnoc")
    )


def main():
    """Run main rfnoc_modtool function."""
    pkg_data_dir = os.path.normpath(get_pkg_data_path())
    cmds = collect_commands()
    args = parse_args(cmds)
    init_logging(log_level=args.log_level)
    if args.directory:
        os.chdir(args.directory)
    cmd = cmds[args.command]
    global_vars = get_global_vars(pkg_data_dir)
    cmd = resolve_vars(cmd, global_vars, args)
    if not cmd.get("skip_identify_module", False):
        if not check_valid_oot_dir(os.getcwd()):
            print("Error: Not a valid OOT module directory")
            return 1
        oot_dir_name = os.path.split(os.getcwd())[-1]
        module_name = oot_dir_name.replace("rfnoc-", "")
        global_vars["MODULE_NAME"] = module_name
        global_vars["MODULE_NAME_FULL"] = oot_dir_name
    executor = StepExecutor(global_vars, args, cmd)
    executor.run(cmd["steps"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
