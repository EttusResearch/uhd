#!/usr/bin/env python3

"""
Copyright (c) 2022 Ettus Research, A National Instruments Brand

SPDX-License-Identifier: GPL-3.0-or-later
"""

import argparse
import inspect
import os
import re
import sys

import uhd
import uhd.usrpctl.commands

def get_commands():
    """
    Returns a dictionary of all subclasses of BaseCommand

    Classes are searched in uhd.usrpctl.commands module.
    Key is the command name of the command class.
    Value is the command class itself.

    BaseCommand is not part of the resulting dictionary
    """
    return {command[1].command_name(): command[1] for command in
            inspect.getmembers(uhd.usrpctl.commands, inspect.isclass)
            if issubclass(command[1], uhd.usrpctl.commands.BaseCommand)
            and command[1] != uhd.usrpctl.commands.BaseCommand}

def parse_args(commands):
    """
    parse command line arguments and return USRPs to search for as well
    as command to execute and command arguments
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("id", nargs="?",
            help="identifies USRPs devices utilizing dev_addr_type")
    parser.add_argument("-v", action="count", default=0,
            help="increase verbosity level ranging from -v to -vvvvv")

    subparsers = parser.add_subparsers(dest="command",
            help="supported commands, detailed help with usrpctl <cmd> --help")

    for command, cls in commands.items():
        cls.add_parser(subparsers)

    args = parser.parse_args()
    script_args = argparse.Namespace()
    script_args.id = args.id
    script_args.v = args.v
    command = args.command
    del args.id
    del args.v
    del args.command
    return (script_args, command, args)

def find_usrps(id_args):
    """
    Find USRPs based on the given id_args.
    """
    return uhd.find(id_args or "")

def main():
    """
    Control all the USRPs!
    """
    commands = get_commands()
    script_args, cmd_name, cmd_args = parse_args(commands)
    env = os.environ
    env['UHD_LOG_CONSOLE_LEVEL'] = str(max(0, 5 - script_args.v))
    usrps = find_usrps(script_args.id)

    if not usrps:
        print(f"No USRP found to act on (id={script_args.id})")
        return 1

    command = commands[cmd_name]()

    if not command.is_multi_device_capable():
        if len(usrps) > 1:
            print(f"Found {len(usrps)} USRPs but {cmd_name} "
                    "can only act on single device")
            return 2

    return command.run(usrps, cmd_args)

if __name__ == "__main__":
    sys.exit(main())
