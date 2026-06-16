#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""MPM utilities for debugging USRPs."""

import multiprocessing
import queue
import signal
from enum import Enum

MPM_RPC_PORT = 49601
MPM_DEFAULT_RPC_TIMEOUT_MS = (
    2000  # Mirrors MPMD_DEFAULT_RPC_TIMEOUT in uhdlib/usrp/common/mpmd_timeouts.hpp
)
MPM_DEFAULT_REBOOT_TIMEOUT_MS = (
    200000  # Mirrors MPMD_DEFAULT_REBOOT_TIMEOUT in uhdlib/usrp/common/mpmd_timeouts.hpp
)
#! Timeout (seconds) for waiting on a claim/unclaim response from the claimer
#  subprocess. Must be long enough to cover subprocess spawn + libpyuhd import
#  + gRPC connection establishment.
#  Mirrors MPMD_CLAIMER_RPC_TIMEOUT in uhdlib/usrp/common/mpmd_timeouts.hpp (converted to seconds).
MPM_CLAIMER_RPC_TIMEOUT_S = 10


def _claim_loop(host, port, cmd_q, token_q):
    """Process that runs a claim loop.

    This function should be run in its own process. Communication to the outside
    process happens with two queues: A command queue, and a token queue. The
    command queue is used to pass in one of these commands: claim, unclaim, or
    exit.
    The token queue is used to read back the current token.
    """

    def _sig_term_handler(_signo, _stack):
        """Gracefully terminate claim loop."""
        cmd_q.put("exit")

    command = None
    token = None
    exit_loop = False

    signal.signal(signal.SIGTERM, _sig_term_handler)

    # The rpc_client must be constructed inside the subprocess — C++ objects
    # cannot be pickled and shared across process boundaries.
    try:
        from uhd import libpyuhd as lib

        print(f"[gRPC] Claim loop connecting to {host}:{port}...")
        client = lib.usrp.rpc_client.make(host, port, MPM_DEFAULT_RPC_TIMEOUT_MS)
        print("[gRPC] Claim loop connected.")
    except Exception as ex:
        print(f"Failed to create RPC client in claim loop: {ex}")
        token_q.put(None)
        return

    try:
        while not exit_loop:
            try:
                command = cmd_q.get(True, 1)
            except queue.Empty:
                # if we do not receive a command, reclaim device
                if token:
                    try:
                        if not client.reclaim():
                            token = None
                            token_q.put(None)
                    except Exception as ex:
                        print(f"Reclaim failed: {ex}")
                        token = None
                        token_q.put(None)
                continue

            if command == "claim":
                if not token:
                    try:
                        token = client.claim("UHD")
                        client.set_token(token)
                    except Exception as ex:
                        # catch RPC errors here so the loop keeps running
                        # and token queue receives an (empty) value
                        print(str(ex))
                else:
                    print("Already have claim")
                token_q.put(token)
            elif command == "unclaim":
                if token:
                    try:
                        client.unclaim()
                    except Exception as ex:
                        print(f"Unclaim failed: {ex}")
                token = None
                token_q.put(None)
            elif command == "exit":
                if token:
                    try:
                        client.unclaim()
                    except Exception as ex:
                        print(f"Unclaim on exit failed: {ex}")
                token = None
                token_q.put(None)
                exit_loop = True
    except Exception as ex:
        print("Unexpected error in claimer loop!")
        print(str(ex))


class MPMClaimer:
    """Holds a claim."""

    def __init__(self, host, port):
        """Initialize the claimer and start the claim loop process."""
        self.token = None
        # Use 'spawn' instead of the default 'fork' start method.
        # gRPC C++ initializes background threads when libpyuhd is imported.
        # Forking after those threads exist causes the child to inherit dead
        # threads and locked mutexes, deadlocking any gRPC call in the child.
        # 'spawn' starts a fresh interpreter with no inherited gRPC state.
        _ctx = multiprocessing.get_context("spawn")
        self._cmd_q = _ctx.Queue()
        self._token_q = _ctx.Queue()
        self._claim_loop = _ctx.Process(
            target=_claim_loop,
            name="Claimer Loop",
            args=(host, port, self._cmd_q, self._token_q),
        )
        self._claim_loop.daemon = True
        self._claim_loop.start()

    def exit(self):
        """Unclaim device and exit claim loop."""
        self.unclaim()
        self._cmd_q.put("exit")
        self._claim_loop.join()

    def unclaim(self):
        """Unclaim device."""
        self._cmd_q.put("unclaim")
        try:
            self.token = self._token_q.get(True, MPM_CLAIMER_RPC_TIMEOUT_S)
        except queue.Empty:
            raise RuntimeError("Timed out waiting for unclaim response from the claim loop process")

    def claim(self):
        """Claim device."""
        self._cmd_q.put("claim")
        try:
            self.token = self._token_q.get(True, MPM_CLAIMER_RPC_TIMEOUT_S)
        except queue.Empty:
            raise RuntimeError("Timed out waiting for claim response from the claim loop process")
        if not self.token:
            raise RuntimeError("Failed to claim device")


class InitMode(Enum):
    """Init modes for MPM session."""

    Hijack = 1
    Claim = 2
    Noclaim = 3


# Ironically, this class will have too many public methods, Pylint just doesn't
# know it yet.
# pylint: disable=too-few-public-methods
class MPMClient:
    """MPM Client: thin Python wrapper around the C++ rpc_client.

    Most MPM RPC methods are forwarded to the underlying C++ client via
    ``__getattr__``. However, ``claim()``, ``unclaim()``, and ``exit()`` are
    intentionally overridden here to manage the local ``MPMClaimer`` and token
    state instead of calling the underlying RPC methods directly.
    The optional MPMClaimer is kept alive for the lifetime of this object so
    the reclaim loop continues running when InitMode.Claim is used. If direct
    access to the underlying RPC client methods is needed, use ``self._client``.
    """

    def __init__(
        self, init_mode, host, port=MPM_RPC_PORT, token=None, timeout_ms=MPM_DEFAULT_RPC_TIMEOUT_MS
    ):
        """Initialize the MPM client and optionally claim the device."""
        assert isinstance(init_mode, InitMode)
        from uhd import libpyuhd as lib

        print(f"[gRPC] Attempting to connect to {host}:{port}...")
        self._claimer = None
        try:
            self._client = lib.usrp.rpc_client.make(host, port, timeout_ms)
        except Exception as ex:
            print(f"[gRPC] Connection refused: {ex}")
            raise
        print("[gRPC] Connection successful.")
        if init_mode == InitMode.Hijack:
            assert token
            self._client.set_token(token)
        elif init_mode == InitMode.Claim:
            self._claimer = MPMClaimer(host, port)
            self._claimer.claim()
            self._client.set_token(self._claimer.token)
        # InitMode.Noclaim: connect without claiming

    def __del__(self):
        """Clean up claimer on object destruction."""
        if self._claimer is not None:
            self._claimer.exit()

    def __getattr__(self, name):
        """Forward all unknown attribute lookups to the underlying C++ client."""
        return getattr(self._client, name)

    def claim(self):
        """Use claimer (instead of RPC method) to claim MPM device."""
        if self._claimer is None:
            raise RuntimeError("Cannot claim: MPMClient was not initialized with InitMode.Claim")
        self._claimer.claim()
        self._client.set_token(self._claimer.token)

    def unclaim(self):
        """Use claimer (instead of RPC method) to unclaim MPM device."""
        if self._claimer is None:
            raise RuntimeError("Cannot unclaim: MPMClient was not initialized with InitMode.Claim")
        self._claimer.unclaim()
        self._client.set_token("")

    def exit(self):
        """Use claimer (instead of RPC method) to unclaim MPM device and exit claim loop."""
        if self._claimer is None:
            return
        self._claimer.exit()


# pylint: enable=too-few-public-methods
