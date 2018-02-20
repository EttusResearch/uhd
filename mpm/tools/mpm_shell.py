#!/usr/bin/env python3
#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
RPC shell to debug USRP MPM capable devices
"""

from __future__ import print_function
import cmd
import time
import argparse
import threading
from importlib import import_module
from mprpc import RPCClient
from mprpc.exceptions import RPCError

try:
    from usrp_mpm.mpmtypes import MPM_RPC_PORT
except ImportError:
    MPM_RPC_PORT = None

DEFAULT_MPM_RPC_PORT = 49601
if MPM_RPC_PORT is None:
    MPM_RPC_PORT = DEFAULT_MPM_RPC_PORT
if MPM_RPC_PORT != DEFAULT_MPM_RPC_PORT:
    print("Warning: Default encoded MPM RPC port does not match that in MPM.")


def parse_args():
    """
    Parse command line args.
    """
    parser = argparse.ArgumentParser(
        description="MPM Shell",
    )
    parser.add_argument(
        'host',
        help="Specify host to connect to.", default=None,
    )
    parser.add_argument(
        '-p', '--port', type=int,
        help="Specify port to connect to.", default=MPM_RPC_PORT,
    )
    parser.add_argument(
        '-c', '--claim',
        action='store_true',
        help="Claim device after connecting."
    )
    parser.add_argument(
        '-j', '--hijack', type=str,
        help="Hijack running session (excludes --claim)."
    )
    return parser.parse_args()


def split_args(args, *default_args):
    " Returns an array of args, space-separated "
    args = args.split()
    return [
        arg_val if arg_idx < len(args) else default_args[arg_idx]
        for arg_idx, arg_val in enumerate(args)
    ]


class MPMClaimer(object):
    """
    Holds a claim.
    """
    def __init__(self, host, port, disc_callback):
        self.token = None
        self._exit_loop = False
        self._disc_callback = disc_callback
        self._claim_loop = threading.Thread(
            target=self.claim_loop,
            name="Claimer Loop",
            args=(host, port, self._disc_callback)
        )
        self._claim_loop.start()

    def claim_loop(self, host, port, disc_callback):
        """
        Run a claim loop
        """
        client = RPCClient(host, port, pack_params={'use_bin_type': True})
        self.token = client.call('claim', 'MPM Shell')
        try:
            while not self._exit_loop:
                client.call('reclaim', self.token)
                time.sleep(1)
            client.call('unclaim', self.token)
        except RPCError as ex:
            print("Unexpected RPC error in claimer loop!")
            print(str(ex))
        disc_callback()
        self.token = None

    def unclaim(self):
        """
        Unclaim device and exit claim loop.
        """
        self._exit_loop = True
        self._claim_loop.join()

class MPMHijacker(object):
    """
    Looks like a claimer object, but doesn't actually claim.
    """
    def __init__(self, token):
        self.token = token

    def unclaim(self):
        """
        Unclaim device and exit claim loop.
        """
        pass


class MPMShell(cmd.Cmd):
    """
    RPC Shell class. See cmd module.
    """
    def __init__(self, host, port, claim, hijack):
        cmd.Cmd.__init__(self)
        self.prompt = "> "
        self.client = None
        self.remote_methods = []
        self._claimer = None
        self._host = host
        self._port = port
        self._device_info = None
        if host is not None:
            self.connect(host, port)
            if claim:
                self.claim()
            elif hijack:
                self.hijack(hijack)
        self.update_prompt()

    def _add_command(self, command, docs, requires_token=False):
        """
        Add a command to the current session
        """
        cmd_name = 'do_' + command
        if not hasattr(self, cmd_name):
            new_command = lambda args: self.rpc_template(
                str(command), requires_token, args
            )
            new_command.__doc__ = docs
            setattr(self, cmd_name, new_command)
            self.remote_methods.append(command)

    def rpc_template(self, command, requires_token, args=None):
        """
        Template function to create new RPC shell commands
        """
        if requires_token and \
                (self._claimer is None or self._claimer.token is None):
            print("Cannot execute `{}' -- no claim available!")
            return
        try:
            if args or requires_token:
                expanded_args = self.expand_args(args)
                if requires_token:
                    expanded_args.insert(0, self._claimer.token)
                response = self.client.call(command, *expanded_args)
            else:
                response = self.client.call(command)
        except RPCError as ex:
            print("RPC Command failed!")
            print("Error: {}".format(ex))
            return
        except Exception as ex:
            print("Unexpected exception!")
            print("Error: {}".format(ex))
            return
        if isinstance(response, bool):
            if response:
                print("Command executed successfully!")
            else:
                print("Command failed!")
        else:
            print("==> " + str(response))
        return response

    def get_names(self):
        " We need this for tab completion. "
        return dir(self)

    ###########################################################################
    # Cmd module specific
    ###########################################################################
    def run(self):
        " Go, go, go! "
        try:
            self.cmdloop()
        except KeyboardInterrupt:
            self.do_disconnect(None)
            exit(0)

    def postcmd(self, stop, line):
        """
        Is run after every command executes. Does:
        - Update prompt
        """
        self.update_prompt()

    ###########################################################################
    # Internal methods
    ###########################################################################
    def connect(self, host, port):
        """
        Launch a connection.
        """
        print("Attempting to connect to {host}:{port}...".format(
            host=host, port=port
        ))
        try:
            self.client = RPCClient(host, port, pack_params={'use_bin_type': True})
            print("Connection successful.")
        except Exception as ex:
            print("Connection refused")
            print("Error: {}".format(ex))
            return False
        self._host = host
        self._port = port
        print("Getting methods...")
        methods = self.client.call('list_methods')
        for method in methods:
            self._add_command(*method)
        print("Added {} methods.".format(len(methods)))
        print("Quering device info...")
        self._device_info = self.client.call('get_device_info')
        return True

    def disconnect(self):
        """
        Clean up after a connection was closed.
        """
        self._device_info = None
        if self._claimer is not None:
            self._claimer.unclaim()
        if self.client:
            try:
                self.client.close()
            except RPCError as ex:
                print("Error while closing the connection")
                print("Error: {}".format(ex))
        for method in self.remote_methods:
            delattr(self, "do_" + method)
        self.remote_methods = []
        self.client = None
        self._host = None
        self._port = None

    def claim(self):
        " Initialize claim "
        assert self.client is not None
        if self._claimer is not None:
            print("Claimer already active.")
            return True
        print("Claiming device...")
        self._claimer = MPMClaimer(self._host, self._port, self.unclaim_hook)
        return True

    def hijack(self, token):
        " Hijack running session "
        assert self.client is not None
        if self._claimer is not None:
            print("Claimer already active. Can't hijack.")
            return False
        print("Hijacking device...")
        self._claimer = MPMHijacker(token)
        return True

    def unclaim(self):
        """
        unclaim
        """
        if self._claimer is not None:
            self._claimer.unclaim()
            self._claimer = None

    def unclaim_hook(self):
        """
        Hook
        """
        pass

    def update_prompt(self):
        """
        Update prompt
        """
        if self._device_info is None:
            self.prompt = '> '
        else:
            if self._claimer is None:
                claim_status = ''
            elif isinstance(self._claimer, MPMClaimer):
                claim_status = ' [C]'
            elif isinstance(self._claimer, MPMHijacker):
                claim_status = ' [H]'
            self.prompt = '{dev_id}{claim_status}> '.format(
                dev_id=self._device_info.get(
                    'name', self._device_info.get('serial', '?')
                ),
                claim_status=claim_status,
            )

    def expand_args(self, args):
        """
        Takes a string and returns a list
        """
        if self._claimer is not None and self._claimer.token is not None:
            args = args.replace('$T', str(self._claimer.token))
        eval_preamble = '='
        args = args.strip()
        if args.startswith(eval_preamble):
            parsed_args = eval(args.lstrip(eval_preamble))
            if not isinstance(parsed_args, list):
                parsed_args = [parsed_args]
        else:
            parsed_args = []
            for arg in args.split():
                try:
                    parsed_args.append(int(arg, 0))
                    continue
                except ValueError:
                    pass
                try:
                    parsed_args.append(float(arg))
                    continue
                except ValueError:
                    pass
                parsed_args.append(arg)
        return parsed_args

    ###########################################################################
    # Predefined commands
    ###########################################################################
    def do_connect(self, args):
        """
        Connect to a remote MPM server. See connect()
        """
        host, port = split_args(args, 'localhost', MPM_RPC_PORT)
        port = int(port)
        self.connect(host, port)

    def do_claim(self, _):
        """
        Spawn a claim loop
        """
        self.claim()

    def do_hijack(self, token):
        """
        Hijack a running session
        """
        self.hijack(token)

    def do_unclaim(self, _):
        """
        unclaim
        """
        self.unclaim()

    def do_disconnect(self, _):
        """
        disconnect from the RPC server
        """
        self.disconnect()

    def do_import(self, args):
        """import a python module into the global namespace"""
        globals()[args] = import_module(args)

    def do_EOF(self, _):
        " When catching EOF, exit the program. "
        print("Exiting...")
        self.disconnect()
        exit(0)

def main():
    " Go, go, go! "
    args = parse_args()
    my_shell = MPMShell(args.host, args.port, args.claim, args.hijack)

    try:
        return my_shell.run()
    except KeyboardInterrupt:
        my_shell.disconnect()
    except Exception as ex:
        print("Uncaught exception: " + str(ex))
        my_shell.disconnect()
    return True

if __name__ == "__main__":
    exit(not main())

