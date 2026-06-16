"""Copyright (c) 2023 Ettus Research, A National Instruments Brand.

SPDX-License-Identifier: GPL-3.0-or-later
"""

from uhd.utils.mpmtools import (
    InitMode,
    MPM_DEFAULT_REBOOT_TIMEOUT_MS,
    MPM_RPC_PORT,
    MPMClient,
)

from .command import BaseCommand


class ResetCommand(BaseCommand):
    """Command that resets the specified subcomponents of the USRP device."""

    @classmethod
    def add_parser(cls, parser):
        """Allow --mpm as an argument.

        Other subcomponents and full device reset not supported right now.
        """
        subparser = parser.add_parser(
            cls.command_name(), help="reset the specified subcomponent(s) on the USRP device"
        )
        subparser.add_argument(
            "--mpm", const="mpm", action="store_const", help="reset MPM on the USRP device"
        )

    def is_multi_device_capable(self):
        """Can handle multiple USRPs."""
        return True

    def run(self, usrps, args):
        """Reset the specified subcomponents for the USRP device(s)."""
        args_list = self._to_arg_list(args)
        if not args_list:
            print("No subcomponent specified for reset, reset did nothing")
            return
        for usrp in usrps:
            if "mpm" in args_list:
                client = None
                try:
                    if "mgmt_addr" not in usrp:
                        # For devices that don't support MPM (e.g. x310)
                        print("Device does not have mgmt_addr, MPM not reset")
                        continue
                    client = MPMClient(
                        InitMode.Claim,
                        usrp["mgmt_addr"],
                        port=MPM_RPC_PORT,
                        timeout_ms=MPM_DEFAULT_REBOOT_TIMEOUT_MS,
                    )
                    print(f"Resetting MPM for device at {usrp['mgmt_addr']}...")
                    client.reset_timer_and_mgr()
                    print("MPM reset successfully")
                except Exception as e:
                    print(f"Resetting MPM was unsuccessful: {e}")
                finally:
                    if client is not None:
                        client.exit()
