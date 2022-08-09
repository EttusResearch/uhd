"""
Copyright (c) 2022 Ettus Research, A National Instruments Brand

SPDX-License-Identifier: GPL-3.0-or-later
"""

from .command import BaseCommand

class ProbeCommand(BaseCommand):
    """
    Command that uses uhd_usrp_probe to query device properties.
    Acts on single devices only.
    """
    @classmethod
    def add_parser(cls, parser):
        """
        Allow -tree as argument. other uhd_usrp_probe arguments
        are not supported right now
        """
        subparser = parser.add_parser(cls.command_name(),
                help="report detailed information on USRP device")
        subparser.add_argument("-tree", const="tree", action="store_const",
                help="reads the device tree")

    def _run_commands(self, usrp, args):
        """
        probe the device
        """
        yield from self._external_process(
                ["uhd_usrp_probe", f"--args={usrp.to_string()}"] +
                self._to_arg_list(args))
