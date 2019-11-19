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
import re
import cmd
import time
import argparse
import multiprocessing
from importlib import import_module

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
    parser.add_argument(
        '-s', '--script', type=str,
        help="Run shell in scripting mode. Specified script contains "
             "MPM shell commands, one per line."
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
    def __init__(self, host, port):
        self.token = None
        self.hijacked = False
        self._cmd_q = multiprocessing.Queue()
        self._token_q = multiprocessing.Queue()
        self._claim_loop = multiprocessing.Process(
            target=self.claim_loop,
            name="Claimer Loop",
            args=(host, port, self._cmd_q, self._token_q)
        )
        self._claim_loop.start()

    def claim_loop(self, host, port, cmd_q, token_q):
        """
        Run a claim loop
        """
        from mprpc import RPCClient
        from mprpc.exceptions import RPCError
        command = None
        token = None
        exit_loop = False
        client = RPCClient(host, port, pack_params={'use_bin_type': True})
        try:
            while not exit_loop:
                if token and not command:
                    client.call('reclaim', token)
                elif command == 'claim':
                    if not token:
                        token = client.call('claim', 'MPM Shell')
                    else:
                        print("Already have claim")
                    token_q.put(token)
                elif command == 'unclaim':
                    if token:
                        client.call('unclaim', token)
                    token = None
                    token_q.put(None)
                elif command == 'exit':
                    if token:
                        client.call('unclaim', token)
                    token = None
                    token_q.put(None)
                    exit_loop = True
                time.sleep(1)
                command = None
                if not cmd_q.empty():
                    command = cmd_q.get(False)
        except RPCError as ex:
            print("Unexpected RPC error in claimer loop!")
            print(str(ex))

    def exit(self):
        """
        Unclaim device and exit claim loop.
        """
        self.unclaim()
        self._cmd_q.put('exit')
        self._claim_loop.join()

    def unclaim(self):
        """
        Unclaim device.
        """
        if not self.hijacked:
            self._cmd_q.put('unclaim')
        else:
            self.hijacked = False
        self.token = None

    def claim(self):
        """
        Claim device.
        """
        self._cmd_q.put('claim')
        self.token = self._token_q.get(True, 5.0)

    def get_token(self):
        """
        Get current token (if any)
        """
        if not self._token_q.empty():
            self.token = self._token_q.get(False)
        return self.token

    def hijack(self, token):
        """
        Take over existing session by providing session token.
        """
        if self.token:
            print("Already have token")
            return
        else:
            self.token = token
        self.hijacked = True

class MPMShell(cmd.Cmd):
    """
    RPC Shell class. See cmd module.
    """
    def __init__(self, host, port, claim, hijack, script):
        cmd.Cmd.__init__(self)
        self.prompt = "> "
        self.client = None
        self.remote_methods = []
        self._host = host
        self._port = port
        self._device_info = None
        self._claimer = MPMClaimer(self._host, self._port)
        if host is not None:
            self.connect(host, port)
            if claim:
                self.claim()
            elif hijack:
                self.hijack(hijack)
        self.update_prompt()
        self._script = script
        if self._script:
            self.parse_script()

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

    def _print_response(self, response):
        print(re.sub("^", "< ", response, flags=re.MULTILINE))

    def rpc_template(self, command, requires_token, args=None):
        """
        Template function to create new RPC shell commands
        """
        from mprpc.exceptions import RPCError
        if requires_token and \
                (self._claimer is None or self._claimer.get_token() is None):
            self._print_response("Cannot execute `{}' -- "
                                 "no claim available!".format(command))
            return False
        try:
            if args or requires_token:
                expanded_args = self.expand_args(args)
                if requires_token:
                    expanded_args.insert(0, self._claimer.get_token())
                response = self.client.call(command, *expanded_args)
            else:
                response = self.client.call(command)
        except RPCError as ex:
            self._print_response("RPC Command failed!\nError: {}".format(ex))
            return False
        except Exception as ex:
            self._print_response("Unexpected exception!\nError: {}".format(ex))
            return True
        if isinstance(response, bool):
            if response:
                self._print_response("Command succeeded.")
            else:
                self._print_response("Command failed!")
        else:
            self._print_response(str(response))

        return False

    def get_names(self):
        " We need this for tab completion. "
        return dir(self)

    ###########################################################################
    # Cmd module specific
    ###########################################################################
    def default(self, line):
        self._print_response("*** Unknown syntax: %s" % line)

    def preloop(self):
        """
        In script mode add Execution start marker to ease parsing script output
        :return: None
        """
        if self._script:
            print("Execute %s" % self._script)

    def precmd(self, line):
        """
        Add command prepended by "> " in scripting mode to ease parsing script
        output.
        """
        if self.cmdqueue:
            print("> %s" % line)
        return line

    def postcmd(self, stop, line):
        """
        Is run after every command executes. Does:
        - Update prompt
        """
        self.update_prompt()
        return stop

    ###########################################################################
    # Internal methods
    ###########################################################################
    def connect(self, host, port):
        """
        Launch a connection.
        """
        from mprpc import RPCClient
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
        from mprpc.exceptions import RPCError
        self._device_info = None
        if self._claimer is not None:
            self._claimer.exit()
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
        print("Claiming device...")
        self._claimer.claim()
        return True

    def hijack(self, token):
        " Hijack running session "
        if self._claimer.hijacked:
            print("Claimer already active. Can't hijack.")
            return False
        print("Hijacking device...")
        self._claimer.hijack(token)
        return True

    def unclaim(self):
        """
        unclaim
        """
        self._claimer.unclaim()

    def update_prompt(self):
        """
        Update prompt
        """
        if self._device_info is None:
            self.prompt = '> '
        else:
            token = self._claimer.get_token()
            if token is None:
                claim_status = ''
            elif self._claimer.hijacked:
                claim_status = ' [H]'
            else:
                claim_status = ' [C]'
            self.prompt = '{dev_id}{claim_status}> '.format(
                dev_id=self._device_info.get(
                    'name', self._device_info.get('serial', '?')
                ),
                claim_status=claim_status,
            )

    def parse_script(self):
        """
        Adding script command from file pointed to by self._script.

        The commands are read from file one per line and added to cmdqueue of
        parent class. This way they will be executed instead of input from
        stdin. An EOF command is appended to the list to ensure the shell exits
        after script execution.
        :return: None
        """
        try:
            with open(self._script, "r") as script:
                for command in script:
                    self.cmdqueue.append(command.strip())
        except OSError as ex:
            print("Failed to read script. (%s)" % ex)
        self.cmdqueue.append("EOF") # terminate shell after script execution

    def expand_args(self, args):
        """
        Takes a string and returns a list
        """
        if self._claimer is not None and self._claimer.get_token() is not None:
            args = args.replace('$T', str(self._claimer.get_token()))
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

    # pylint: disable=invalid-name
    def do_EOF(self, _):
        """
        When catching EOF, exit the program.
        """
        print("Exiting...")
        self.disconnect()
        return True # orderly shutdown

def main():
    " Go, go, go! "
    args = parse_args()
    my_shell = MPMShell(args.host, args.port, args.claim,
                        args.hijack, args.script)

    try:
        my_shell.cmdloop()
    except KeyboardInterrupt:
        my_shell.disconnect()
    except Exception as ex: # pylint: disable=broad-except
        print("Uncaught exception: " + str(ex))
        my_shell.disconnect()
        return False
    return True

if __name__ == "__main__":
    exit(not main())
