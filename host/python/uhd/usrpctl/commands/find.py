"""
Copyright (c) 2022 Ettus Research, A National Instruments Brand

SPDX-License-Identifier: GPL-3.0-or-later
"""

from .command import BaseCommand

class FindCommand(BaseCommand):
    """
    Class that finds USRP devices
    """

    @classmethod
    def add_parser(cls, parser):
        """
        Adding subparser for find command
        """
        parser.add_parser(cls.command_name(),
                help="print devices found using id parameter")

    def is_multi_device_capable(self):
        """
        Can handle multiple USRPs
        """
        return True

    def run(self, usrps, args):
        """
        Because usrpctl issued find to build the usrps list
        from the id argument the only thing left to do is
        print the found devices. Print mimics the behaviour
        of uhd_find_devices.
        """
        for index, usrp in enumerate(usrps):
            print('--------------------------------------------------')
            print(f"-- UHD Device {index}")
            print('--------------------------------------------------')
            print('Device Address:')
            for key, value in usrp.to_dict().items():
                print(f"    {key}: {value}")
            print()
            print()
