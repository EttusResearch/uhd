#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
RPC/MPM utilities for debugging USRPs
"""

from enum import Enum
import multiprocessing
import queue
import signal
from mprpc import RPCClient
from mprpc.exceptions import RPCError

MPM_RPC_PORT = 49601

def _claim_loop(client, cmd_q, token_q):
    """
    Process that runs a claim loop.

    This function should be run in its own thread. Communication to the outside
    thread happens with two queues: A command queue, and a token queue. The
    command queue is used to pass in one of these commands: claim, unclaim, or
    exit.
    The token queue is used to read back the current token.
    """

    def _sig_term_handler(_signo, _stack):
        """
        gracefully terminate claim loop
        """
        cmd_q.put("exit")

    command = None
    token = None
    exit_loop = False

    signal.signal(signal.SIGTERM, _sig_term_handler)

    try:
        while not exit_loop:
            try:
                command = cmd_q.get(True, 1)
            except queue.Empty:
                # if we do not receive a command, reclaim device
                if token:
                    client.call('reclaim', token)
                continue

            if command == 'claim':
                if not token:
                    try:
                        token = client.call('claim', 'UHD')
                    except RPCError as ex:
                        # catch RPC errors here so the loop keeps running
                        # and token queue receives an (empty) value
                        print(str(ex))
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
    except RPCError as ex:
        print("Unexpected RPC error in claimer loop!")
        print(str(ex))

class MPMClaimer:
    """
    Holds a claim.
    """
    def __init__(self, host, port):
        self.token = None
        self._cmd_q = multiprocessing.Queue()
        self._token_q = multiprocessing.Queue()
        client = RPCClient(host, port, pack_params={'use_bin_type': True})
        self._claim_loop = multiprocessing.Process(
            target=_claim_loop,
            name="Claimer Loop",
            args=(client, self._cmd_q, self._token_q)
        )
        self._claim_loop.daemon = True
        self._claim_loop.start()

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
        self._cmd_q.put('unclaim')
        self.token = self._token_q.get(True, 1)

    def claim(self):
        """
        Claim device.
        """
        self._cmd_q.put('claim')
        self.token = self._token_q.get(True, 1)
        if not self.token:
            raise RuntimeError("Failed to claim device")

    def get_token(self):
        """
        Get current token (if any)
        """
        if not self._token_q.empty():
            self.token = self._token_q.get(False)
        return self.token

class InitMode(Enum):
    """
    Init modes for MPM session
    """
    Hijack = 1
    Claim = 2
    Noclaim = 3

# Ironically, this class will have too many public methods, Pylint just doesn't
# know it yet.
# pylint: disable=too-few-public-methods
class MPMClient:
    """
    MPM RPC Client: Will make all MPM commands accessible as Python methods.
    """
    def __init__(self, init_mode, host, port=MPM_RPC_PORT, token=None):
        assert isinstance(init_mode, InitMode)
        print("[MPMRPC] Attempting to connect to {host}:{port}...".format(
            host=host, port=port
        ))
        self._remote_methods = []
        if init_mode == InitMode.Hijack:
            assert token
            self._token = token
        if init_mode == InitMode.Claim:
            self._claimer = MPMClaimer(host, port)
            self._claimer.claim()
            self._token = self._claimer.token

        try:
            self._client = RPCClient(host, port, pack_params={'use_bin_type': True})
            print("[MPMRPC] Connection successful.")
        except Exception as ex:
            print("[MPMRPC] Connection refused: {}".format(ex))
            raise RuntimeError("RPC connection refused.")
        print("[MPMRPC] Getting methods...")
        methods = self._client.call('list_methods')
        for method in methods:
            self._add_command(*method)
        print("[MPMRPC] Added {} methods.".format(len(methods)))

    def __del__(self):
        if hasattr(self, '_claimer'):
            self._claimer.exit()

    def claim(self):
        """
        Use claimer (instead of RPC method) to claim MPM device
        """
        self._claimer.claim()

    def unclaim(self):
        """
        Use claimer (instead of RPC method) to unclaim MPM device
        """
        self._claimer.unclaim()

    def exit(self):
        """
        Use claimer (instead of RPC method) to unclaim MPM device and exit claim loop
        """
        self._claimer.exit()

    def _add_command(self, command, docs, requires_token):
        """
        Add a command to the current session
        """
        if not hasattr(self, command):
            new_command = lambda *args, **kwargs: self._rpc_template(
                str(command), requires_token, *args, **kwargs
            )
            new_command.__doc__ = docs
            setattr(self, command, new_command)
            self._remote_methods.append(command)

    def _rpc_template(self, command, requires_token, *args, **kwargs):
        """
        Template function to create new RPC shell commands
        """
        if requires_token and not self._token:
            raise RuntimeError(
                "[MPMRPC] Cannot execute `{}' -- no claim available!"
                .format(command))
        # Put token as the first argument if required:
        if requires_token:
            args = (self._token,) + args
        if kwargs:
            return self._client.call(command, *args, **kwargs)
        if args:
            return self._client.call(command, *args)
        return self._client.call(command)
# pylint: enable=too-few-public-methods
