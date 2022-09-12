#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Implemented RPC Servers
"""

from __future__ import print_function
import traceback
import copy
from random import choice
from string import ascii_letters, digits
from multiprocessing import Process
import threading
import sys
from gevent.server import StreamServer
from gevent.pool import Pool
from gevent import signal
from gevent import spawn_later
from gevent import Greenlet
from gevent import monkey
monkey.patch_all()
from contextlib import contextmanager
from mprpc import RPCServer
from usrp_mpm.mpmlog import get_main_logger
from usrp_mpm.mpmutils import to_binary_str
from usrp_mpm.sys_utils import watchdog
from usrp_mpm.sys_utils import net

TIMEOUT_INTERVAL = 5.0 # Seconds before claim expires (default value)
TOKEN_LEN = 16 # Length of the token string
# Compatibility number for MPM
MPM_COMPAT_NUM = (4, 2)

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
    default_claimed_methods = ['init', 'update_component', 'reclaim', 'unclaim',
                               'get_log_buf']

    ###########################################################################
    # RPC Server Initialization
    ###########################################################################
    def __init__(self, state, default_args):
        self.log = get_main_logger().getChild('RPCServer')
        self.log.trace("Launching RPC server with compat num %d.%d",
                       MPM_COMPAT_NUM[0], MPM_COMPAT_NUM[1])
        self._state = state
        self._timer = Greenlet()
        # Setting this to True will disable an unclaim on timeout. Use with
        # care, and make sure to set it to False again when finished.
        self._disable_timeouts = False
        self._timeout_interval = float(default_args.get(
            "rpc_timeout_interval",
            TIMEOUT_INTERVAL
        ))
        self.session_id = None
        # Create the periph_manager for this device
        # This call will be forwarded to the device specific implementation
        # e.g. in periph_manager/n3xx.py
        # Which implementation is called will be determined during
        # configuration with cmake (-DMPM_DEVICE).
        # mgr is thus derived from PeriphManagerBase
        # (see periph_manager/base.py)
        from usrp_mpm.periph_manager import periph_manager
        self._mgr_generator = lambda: periph_manager(default_args)
        self.periph_manager = self._mgr_generator()
        device_info = self.periph_manager.get_device_info()
        self._state.dev_type.value = \
                to_binary_str(device_info.get("type", "n/a"))
        self._state.dev_product.value = \
                to_binary_str(device_info.get("product", "n/a"))
        self._state.dev_serial.value = \
                to_binary_str(device_info.get("serial", "n/a"))
        self._state.dev_name.value = \
                to_binary_str(device_info.get("name", "n/a"))
        self._state.dev_fpga_type.value = \
                to_binary_str(device_info.get("fpga", "n/a"))
        self._db_methods = []
        self._mb_methods = []
        self.claimed_methods = copy.copy(self.default_claimed_methods)
        self._last_error = ""
        self._init_rpc_calls(self.periph_manager)
        # We call the server __init__ function here, and not earlier, because
        # first the commands need to be registered
        super(MPMServer, self).__init__(
            pack_params={'use_bin_type': True},
            unpack_params={'max_buffer_size': 50000000, 'raw': False},
        )
        self._state.system_ready.value = True
        self.log.info("RPC server ready!")
        # Optionally spawn watchdog. Note: In order for us to be able to spawn
        # the task from this thread, the main process needs to hand control to
        # us using watchdog.transfer_control().
        if watchdog.has_watchdog():
            self.log.info("Spawning watchdog task...")
            watchdog.spawn_watchdog_task(self._state, self.log)

    def _init_rpc_calls(self, mgr):
        """
        Register all RPC calls for the motherboard and daughterboards.

        First clears out all previously registered RPC calls.
        """
        # Clear old calls:
        for meth_list in (self._db_methods, self._mb_methods):
            for method in meth_list:
                if hasattr(self, method):
                    delattr(self, method)
                else:
                    self.log.warning(
                        "Attempted to remove non-existent method: %s",
                        method
                    )
        self._db_methods = []
        self._mb_methods = []
        # Register new ones:
        self._update_component_commands(mgr, '', '_mb_methods')
        for db_slot, dboard in enumerate(mgr.dboards):
            cmd_prefix = 'db_' + str(db_slot) + '_'
            self._update_component_commands(dboard, cmd_prefix, '_db_methods')
        self.log.debug(
            "Registered %d motherboard methods, %d daughterboard methods.",
            len(self._mb_methods),
            len(self._db_methods),
        )

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
                # Because we can only reach this point with a valid claim,
                # there's no harm in resetting the timer
                self._reset_timer()
                return function(*args)
            except Exception as ex:
                self.log.error(
                    "Uncaught exception in method %s: %s \n %s ",
                    command, str(ex), traceback.format_exc()
                )
                self._last_error = str(ex)
                raise
            finally:
                if not self._state.claim_status.value:
                    self.log.error("Lost claim during API call to `%s'!",
                                   command)
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
                    "Uncaught exception in method %s :%s\n %s ",
                    command, str(ex), traceback.format_exc()
                )
                self._last_error = str(ex)
                raise
        new_unclaimed_function.__doc__ = function.__doc__
        setattr(self, command, new_unclaimed_function)

    ###########################################################################
    # Diagnostics and introspection
    ###########################################################################
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

    ###########################################################################
    # Claiming logic
    ###########################################################################
    def _check_token_valid(self, token):
        """
        Returns True iff:
        - The device is currently claimed
        - The claim token matches the one passed in
        """
        token = to_binary_str(token)
        return self._state.claim_status.value and \
                len(token) == TOKEN_LEN and \
                self._state.claim_token.value == token

    def claim(self, session_id):
        """Claim device

        Tries to claim MPM device and provides a human readable session_id.
        The caller must remember this token, and call reclaim() on regular
        intervals in order not to lose the claim.

        Will return a token on success, or raise an Exception on failure.
        """
        self._state.lock.acquire()
        if self._state.claim_status.value:
            error_msg = \
                "Someone tried to claim this device again (From: {})".format(
                    self.client_host)
            self.log.warning(error_msg)
            self._last_error = error_msg
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
        self.periph_manager.claimed = True
        self.periph_manager.claim()
        if self.periph_manager.clear_rpc_registry_on_unclaim:
            self._init_rpc_calls(self.periph_manager)
        self._state.lock.release()
        self.session_id = session_id + " ({})".format(self.client_host)
        self._reset_timer()
        self.log.debug(
            "giving token: %s to host: %s",
            self._state.claim_token.value,
            self.client_host
        )
        if _is_connection_local(self.client_host):
            self.periph_manager.set_connection_type("local")
        else:
            self.periph_manager.set_connection_type("remote")
        return self._state.claim_token.value

    def reclaim(self, token):
        """
        Reclaim a MPM device with a token. This operation will fail if the
        device is claimed and the token doesn't match, or if the device is not
        claimed at all.
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
        Unconditional unclaim - for internal use

        Resets and deinitalizes the periph manager as well.
        """
        self._state.lock.acquire()
        self.log.debug(
            "Deinitializing device and releasing claim on session `{}'"
            .format(self.session_id))
        # Disable unclaim timer, we're now finished with reclaim loops.
        self._timer.kill(block=False)
        # We might need to clear the method registry
        if self.periph_manager.clear_rpc_registry_on_unclaim:
            self.clear_method_registry()
        # Now unclaim and deinit the device. We will try and catch any exception
        # here, because the session is over and we have nowhere to send the
        # exception.
        try:
            self.periph_manager.claimed = False
            self.periph_manager.unclaim()
            self.periph_manager.set_connection_type(None)
            self.periph_manager.deinit()
        except BaseException as ex:
            self._last_error = str(ex)
            self.log.error("Deinitialization failed: %s", str(ex))
            # Don't want to propagate this failure -- the session is over
        finally:
            # The finally clause is not strictly necessary, because we're catching
            # everything and not returning, but it should be explicit that we
            # must always clear the claim and the _state lock at this point.
            self._state.claim_status.value = False
            self._state.claim_token.value = b''
            self._state.lock.release()
            self.session_id = None

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

    def _timeout_event(self):
        " Callback for the claim timeout. "
        if self._disable_timeouts:
            self.log.debug("Timeouts are disabled: Snoozing")
            self._reset_timer()
        else:
            self.log.warning("A timeout event occured!")
            self._unclaim()

    def _reset_timer(self):
        """
        Reset unclaim timer. After calling this, call this function again
        within 'timeout' seconds to avoid a timeout event.
        """
        self._timer.kill()
        self._timer = spawn_later(self._timeout_interval, self._timeout_event)

    @contextmanager
    def _timeout_disabler(self):
        self._disable_timeouts = True
        try:
            yield self
        finally:
            self._disable_timeouts = False

    ###########################################################################
    # Status queries
    ###########################################################################
    def get_mpm_compat_num(self):
        """Get the MPM compatibility number"""
        return MPM_COMPAT_NUM

    def get_device_info(self):
        """
        get device information
        This is as safe method which can be called without a claim on the device
        """
        info = self.periph_manager.get_device_info()
        info["mpm_version"] = "{}.{}".format(*MPM_COMPAT_NUM)
        if _is_connection_local(self.client_host):
            info["connection"] = "local"
        else:
            info["connection"] = "remote"
        return info

    def get_last_error(self):
        """
        Return the 'last error' string, which gets set when RPC calls fail.
        """
        return self._last_error

    def get_log_buf(self, token):
        """
        Return the contents of the log buffer as a list of str -> str
        dictionaries.
        """
        if not self._check_token_valid(token):
            self.log.warning(
                "Attempt to read logs without valid claim from {}".format(
                    self.client_host
                )
            )
            err_msg = "get_log_buf() called without valid claim."
            self._last_error = err_msg
            raise RuntimeError(err_msg)
        log_records = get_main_logger().get_log_buf()
        self.log.trace("Returning %d log records.", len(log_records))
        return [
            {k: str(v) for k, v in iter(record.items())}
            for record in log_records
        ]

    ###########################################################################
    # Session initialization
    ###########################################################################
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
        try:
            result = self.periph_manager.init(args)
        except Exception as ex:
            self._last_error = str(ex)
            self.log.error("init() failed with error: %s", str(ex))
        finally:
            self.log.debug("init() result: {}".format(result))
        return result

    ###########################################################################
    # Update components
    ###########################################################################
    def clear_method_registry(self):
        """
        Clear all the methods in the RPC server method cache.
        """
        # RPCServer caches RPC methods, but that cache is not accessible here
        # (because Cython). Re-running `RPCServer.__init__` clears that cache,
        # and allows us to register new RPC methods.
        # A note on maintenance: This has been deemed safe through inspection of
        # the RPCServer source code. However, this is not typical Python, and
        # changes in future versions of RPCServer may cause issues.
        super(MPMServer, self).__init__(
            pack_params={'use_bin_type': True},
            unpack_params={'max_buffer_size': 50000000, 'raw': False},
        )

    def _reset_mgr(self):
        """
        Reset the Peripheral Manager for this RPC server.
        """
        self.log.info("Resetting peripheral manager.")
        self.periph_manager.tear_down()
        self.periph_manager = None
        self.periph_manager = self._mgr_generator()
        self._init_rpc_calls(self.periph_manager)
        # Clear the method cache in order to remove stale references to
        # methods from the old peripheral manager (the one before reset)
        self.clear_method_registry()
        # update the FPGA type information in the state
        device_info = self.periph_manager.get_device_info()
        self._state.dev_fpga_type.value = \
                to_binary_str(device_info.get("fpga", "n/a"))

    def reset_timer_and_mgr(self, token):
        """
        Pause the timers, reset the peripheral manager and restart the
        timers.
        """
        # Check the claimed status
        if not self._check_token_valid(token):
            self._last_error =\
                "Attempt to reset manager without valid claim from {}".format(
                    self.client_host
                )
            self.log.error(self._last_error)
            raise RuntimeError("Attempt to reset manager without valid claim.")

        # Stop the timer, reset_timer_and_mgr can take some time:
        with self._timeout_disabler():
            try:
                self._reset_mgr()
                self.log.debug("Reset the periph manager")
            except Exception as ex:
                self.log.error(
                    "Error in reset_timer_and_mgr: {}".format(
                        ex
                    ))
                self._last_error = str(ex)

        self.log.debug("End of reset_timer_and_mgr")
        self._reset_timer()

    def update_component(self, token, file_metadata_l, data_l):
        """"
        Updates the device component files specified by the metadata and data
        :param file_metadata_l: List of dictionary of strings containing metadata
        :param data_l: List of binary string with the file contents to be written
        """
        # Check the claimed status
        if not self._check_token_valid(token):
            self._last_error =\
                "Attempt to update component without valid claim from {}".format(
                    self.client_host
                )
            self.log.error(self._last_error)
            raise RuntimeError("Attempt to update component without valid claim.")
        with self._timeout_disabler():
            result = self.periph_manager.update_component(file_metadata_l, data_l)
            if not result:
                component_ids = [metadata['id'] for metadata in file_metadata_l]
                raise RuntimeError("Failed to update components: {}".format(component_ids))

            # Check if we need to reset the peripheral manager
            reset_now = False
            for metadata, data in zip(file_metadata_l, data_l):
                # Make sure the component is in the updateable_components
                component_id = metadata['id']
                if component_id in self.periph_manager.updateable_components:
                    # Check if that updating that component means the PM should be reset
                    reset_now = (reset_now or
                                 self.periph_manager.updateable_components[component_id]['reset']) and \
                                 not metadata.get('reset', "").lower() == "false"
                else:
                    self.log.debug("ID {} not in updateable components ({})".format(
                        component_id, self.periph_manager.updateable_components))
            try:
                self.log.trace("Reset after updating component? {}".format(reset_now))
                if reset_now:
                    self._reset_mgr()
                    self.log.debug("Reset the periph manager")
            except Exception as ex:
                self.log.error(
                    "Error in update_component while resetting: {}".format(
                        ex
                    ))
                self._last_error = str(ex)

        self.log.debug("End of update_component")
        self._reset_timer()

def _is_connection_local(client_hostname):
    return client_hostname in net.get_local_ip_addrs()

###############################################################################
# Process control
###############################################################################
def _rpc_server_process(shared_state, port, default_args):
    """
    This is the actual process that's running the RPC server.
    """
    connections = Pool(1000)
    server = StreamServer(
        ('0.0.0.0', port),
        handle=MPMServer(shared_state, default_args),
        spawn=connections)
    # catch signals and stop the stream server
    # Previously, the signal callbacks simply called server.stop()
    # gevent doesn't like this because server.stop() may block waiting
    # for greenlets to stop, and signal callbacks are not supposed to block
    stop_event = threading.Event()
    def stop_worker():
        stop_event.wait()
        server.stop()
        sys.exit(0)
    threading.Thread(target=stop_worker, daemon=True).start()
    signal.signal(signal.SIGTERM, lambda *args: stop_event.set())
    signal.signal(signal.SIGINT, lambda *args: stop_event.set())
    server.serve_forever()


def spawn_rpc_process(state, udp_port, default_args):
    """
    Returns a process that contains the RPC server
    """
    proc = Process(
        target=_rpc_server_process,
        args=[udp_port, state, default_args],
    )
    proc.start()
    return proc
