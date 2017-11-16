#
# Copyright 2017 Ettus Research (National Instruments)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
"""
Implemented RPC Servers
"""

from __future__ import print_function
import copy
from random import choice
from string import ascii_letters, digits
from multiprocessing import Process
from gevent.server import StreamServer
from gevent.pool import Pool
from gevent import signal
from gevent import spawn_later
from gevent import Greenlet
from gevent import monkey
monkey.patch_all()
from builtins import str, bytes
from builtins import range
from mprpc import RPCServer
from .mpmlog import get_main_logger

TIMEOUT_INTERVAL = 3.0 # Seconds before claim expires
TOKEN_LEN = 16 # Length of the token string

def no_claim(func):
    " Decorator for functions that require no token check "
    func._notok = True
    return func

def no_rpc(func):
    " Decorator for functions that should not be exposed via RPC "
    func._norpc = True
    return func

class MPMServer(RPCServer):
    """
    Main MPM RPC class which holds the periph_manager object and translates
    RPC calls to appropiate calls in the periph_manager and dboard_managers.
    """
    # This is a list of methods in this class which require a claim
    default_claimed_methods = ['init', 'reclaim', 'unclaim']

    def __init__(self, state, mgr, *args, **kwargs):
        self.log = get_main_logger().getChild('RPCServer')
        self._state = state
        self._timer = Greenlet()
        self.session_id = None
        self.periph_manager = mgr
        self._db_methods = []
        self._mb_methods = []
        self.claimed_methods = copy.copy(self.default_claimed_methods)
        self._last_error = ""
        self._update_component_commands(mgr, '', '_mb_methods')
        for db_slot, dboard in enumerate(mgr.dboards):
            cmd_prefix = 'db_' + str(db_slot) + '_'
            self._update_component_commands(dboard, cmd_prefix, '_db_methods')
        # We call the server __init__ function here, and not earlier, because
        # first the commands need to be registered
        super(MPMServer, self).__init__(
            *args,
            pack_params={'use_bin_type': True},
            **kwargs
        )

    def _check_token_valid(self, token):
        """
        Returns True iff:
        - The device is currently claimed
        - The claim token matches the one passed in
        """
        try:
            token = bytes(token, 'ascii')
        except TypeError:
            pass

        return self._state.claim_status.value and \
                len(token) == TOKEN_LEN and \
                self._state.claim_token.value == token


    def _update_component_commands(self, component, namespace, storage):
        """
        Detect available methods for an object and add them to the RPC server.

        We skip all private methods, and all methods that use the @no_rpc
        decorator.
        """
        for method_name in (
                m for m in dir(component)
                if not m.startswith('_') \
                    and callable(getattr(component, m)) \
                    and not hasattr(self, m) \
                    and not getattr(getattr(component, m), '_norpc', False)
            ):
            new_rpc_method = getattr(component, method_name)
            command_name = namespace + method_name
            if getattr(new_rpc_method, '_notok', False):
                self._add_safe_command(new_rpc_method, command_name)
            else:
                self._add_claimed_command(new_rpc_method, command_name)
                self.claimed_methods.append(command_name)
            getattr(self, storage).append(command_name)


    def _add_claimed_command(self, function, command):
        """
        Adds a method with the name command to the RPC server
        This command will require an acquired claim on the device, and a valid
        token needs to be passed in for it to not fail.

        If the method does not require a token, use _add_safe_command().
        """
        self.log.trace("adding command %s pointing to %s", command, function)
        def new_claimed_function(token, *args):
            " Define a function that requires a claim token check "
            if not self._check_token_valid(token):
                self.log.warning(
                    "Thwarted attempt to access function `{}' with invalid " \
                    "token `{}'.".format(command, token)
                )
                raise RuntimeError("Invalid token!")
            try:
                return function(*args)
            except Exception as ex:
                self.log.error(
                    "Uncaught exception in method %s: %s",
                    command, str(ex)
                )
                self._last_error = str(ex)
                raise
        new_claimed_function.__doc__ = function.__doc__
        setattr(self, command, new_claimed_function)

    def _add_safe_command(self, function, command):
        """
        Add a safe method which does not require a claim on the device.
        If the method should only be called by claimers, use
        _add_claimed_command().
        """
        self.log.trace("adding safe command %s pointing to %s", command, function)
        def new_unclaimed_function(*args):
            " Define a function that does not require a claim token check "
            try:
                return function(*args)
            except Exception as ex:
                self.log.error(
                    "Uncaught exception in method %s: %s",
                    command, str(ex)
                )
                self._last_error = str(ex)
                raise
        new_unclaimed_function.__doc__ = function.__doc__
        setattr(self, command, new_unclaimed_function)

    def list_methods(self):
        """
        Returns a list of tuples: (method_name, docstring, is claim required)

        Every tuple represents one call that's available over RPC.
        """
        return [
            (
                method,
                getattr(self, method).__doc__,
                method in self.claimed_methods
            )
            for method in dir(self)
            if not method.startswith('_') \
                    and callable(getattr(self, method))
        ]

    def ping(self, data=None):
        """
        Take in data as argument and send it back
        This is a safe method which can be called without a claim on the device
        """
        self.log.debug("I was pinged from: %s:%s", self.client_host, self.client_port)
        return data

    def claim(self, session_id):
        """
        claim `token` - tries to claim MPM device and provides a human readable
        session_id.
        """
        self._state.lock.acquire()
        if self._state.claim_status.value:
            self.log.warning("Someone tried to claim this device again")
            self._last_error = "Someone tried to claim this device again"
            self._state.lock.release()
            raise RuntimeError("Double-claim")
        self.log.debug(
            "Claiming from: %s, Session ID: %s",
            self.client_host,
            session_id
        )
        self._state.claim_token.value = bytes(''.join(
            choice(ascii_letters + digits) for _ in range(TOKEN_LEN)
        ), 'ascii')
        self._state.claim_status.value = True
        self._state.lock.release()
        self.session_id = session_id + " ({})".format(self.client_host)
        self._reset_timer()
        self.log.debug(
            "giving token: %s to host: %s",
            self._state.claim_token.value,
            self.client_host
        )
        if self.client_host in ["127.0.0.1", "::1"]:
            self.periph_manager.set_connection_type("local")
        else:
            self.periph_manager.set_connection_type("remote")
        return self._state.claim_token.value


    def init(self, token, args):
        """
        Initialize device. See PeriphManagerBase for details. This is forwarded
        from here import to give extra control over the claim release timeout.
        """
        if not self._check_token_valid(token):
            self.log.warning(
                "Attempt to init without valid claim from {}".format(
                    self.client_host
                )
            )
            self._last_error = "init() called without valid claim."
            raise RuntimeError("init() called without valid claim.")
        self._timer.kill() # Stop the timer, inits can take some time.
        try:
            result = self.periph_manager.init(args)
        except Exception as ex:
            self._last_error = str(ex)
            self.log.error("init() failed with error: %s", str(ex))
        self.log.debug("init() result: {}".format(result))
        self._reset_timer()
        return result

    def reclaim(self, token):
        """
        reclaim a MPM device with a token. This operation will fail
        if the device is claimed and the token doesn't match.
        Or if the device is not claimed at all.
        """
        if self._state.claim_status.value:
            self._state.lock.acquire()
            if self._check_token_valid(token):
                self._state.lock.release()
                self.log.debug("reclaimed from: %s", self.client_host)
                self._reset_timer()
                return True
            self._state.lock.release()
            self.log.debug(
                "reclaim failed from: %s  Invalid token: %s",
                self.client_host, token[:TOKEN_LEN]
            )
            return False
        self.log.debug(
            "trying to reclaim unclaimed device from: %s",
            self.client_host
        )
        return False

    def _unclaim(self):
        """
        unconditional unclaim - for internal use
        """
        self.log.debug("Releasing claim on session `{}'".format(
            self.session_id
        ))
        self._state.claim_status.value = False
        self._state.claim_token.value = b''
        self.session_id = None
        self.periph_manager.claimed = False
        try:
            self.periph_manager.set_connection_type(None)
            self.periph_manager.deinit()
        except Exception as ex:
            self._last_error = str(ex)
            self.log.error("deinit() failed: %s", str(ex))
            # Don't want to propagate this failure -- the session is over
        self._timer.kill()

    def _reset_timer(self, timeout=TIMEOUT_INTERVAL):
        """
        reset unclaim timer
        """
        self._timer.kill()
        self._timer = spawn_later(timeout, self._unclaim)

    def unclaim(self, token):
        """
        unclaim `token` - unclaims the MPM device if it is claimed with this
        token
        """
        if self._check_token_valid(token):
            self._unclaim()
            return True
        self.log.warning("Attempt to unclaim session with invalid token!")
        return False

    def get_device_info(self):
        """
        get device information
        This is as safe method which can be called without a claim on the device
        """
        info = self.periph_manager.get_device_info()
        if self.client_host in ["127.0.0.1", "::1"]:
            info["connection"] = "local"
        else:
            info["connection"] = "remote"
        return info

    def get_last_error(self):
        """
        Return the 'last error' string, which gets set when RPC calls fail.
        """
        return self._last_error



def _rpc_server_process(shared_state, port, mgr):
    """
    This is the actual process that's running the RPC server.
    """
    connections = Pool(1000)
    server = StreamServer(
        ('0.0.0.0', port),
        handle=MPMServer(shared_state, mgr),
        spawn=connections)
    # catch signals and stop the stream server
    signal(signal.SIGTERM, lambda *args: server.stop())
    signal(signal.SIGINT, lambda *args: server.stop())
    server.serve_forever()


def spawn_rpc_process(state, udp_port, mgr):
    """
    Returns a process that contains the RPC server
    """

    proc_args = [udp_port, state, mgr]
    proc = Process(target=_rpc_server_process, args=proc_args)
    proc.start()
    return proc
