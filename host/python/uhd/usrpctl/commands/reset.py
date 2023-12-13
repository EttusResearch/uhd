"""
Copyright (c) 2023 Ettus Research, A National Instruments Brand

SPDX-License-Identifier: GPL-3.0-or-later
"""

from .command import BaseCommand

MPM_RPC_PORT = 49601

class ResetCommand(BaseCommand):
    """
    Command that resets the specified subcomponents of the USRP device
    """
    @classmethod
    def add_parser(cls, parser):
        """
        Allow -mpm as an argument, other subcomponents and full device reset
        not supported right now
        """
        subparser = parser.add_parser(cls.command_name(),
                help="reset the specified subcomponent(s) on the USRP device")
        subparser.add_argument("-mpm", const="mpm", action="store_const",
                help="reset MPM on the USRP device")

    def is_multi_device_capable(self):
        """
        Can handle multiple USRPs
        """
        return True

    def run(self, usrps, args):
        """
        Reset the specified subcomponents for the USRP device(s)
        """
        args_list = self._to_arg_list(args)
        if not args_list:
            print("No subcomponent specified for reset, reset did nothing")
            return
        for usrp in usrps:
            if 'mpm' in args_list:
                import msgpackrpc
                try:
                    if not 'mgmt_addr' in usrp:
                        # For devices that don't support MPM (e.g. x310)
                        print("Device does not have mgmt_addr, MPM not reset")
                        continue
                    # Increase timeout from default, since MPM restarting can take longer 
                    # than 10 seconds on some devices
                    client = msgpackrpc.Client(msgpackrpc.Address(usrp['mgmt_addr'], MPM_RPC_PORT),
                                               timeout=20)
                    token = client.call('claim', 'usrpctl')
                    print(f"Resetting MPM for device at {usrp['mgmt_addr']}...")
                    client.call('reset_timer_and_mgr', token)
                    print("MPM reset successfully")
                except Exception as e:
                    print(f"Resetting MPM was unsuccessful: {e}")
