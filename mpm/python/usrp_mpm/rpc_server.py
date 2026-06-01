#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""RPC server for MPM."""

import inspect
import os
import signal
import threading
import traceback
import weakref
from concurrent import futures
from contextlib import contextmanager
from multiprocessing import Process, RLock, current_process
from random import choice
from string import ascii_letters, digits

import grpc
from gevent import Greenlet, monkey, spawn_later
from usrp_mpm import pb_utils as pbu
from usrp_mpm.mpmlog import get_main_logger
from usrp_mpm.mpmutils import (
    register_chained_signal_handler,
    set_proc_title,
    to_binary_str,
)
from usrp_mpm.rpc_utils import no_claim
from usrp_mpm.sys_utils import net, watchdog

from . import mpm_server_pb2 as mpm_server__pb2, mpm_server_pb2_grpc

monkey.patch_all()

# Initialize gRPC for gevent compatibility.
# This switches gRPC's C core to use gevent-compatible I/O primitives
# instead of native threads. This helps to avoid conflicts between gRPC's
# threading model and gevent's cooperative multitasking.
import grpc._cython.cygrpc

grpc._cython.cygrpc.init_grpc_gevent()

TIMEOUT_INTERVAL = 5.0  # Seconds before claim expires (default value)
LOCK_ACQ_TIMEOUT = 1  # Seconds to wait for acquiring shared lock (default value)
TOKEN_LEN = 16  # Length of the token string
MPM_COMPAT_NUM = (7, 0)  # Compatibility number for MPM
# Maximum gRPC message size (in bytes) to accommodate large FPGA bitfiles while
# preventing trivial memory exhaustion / DoS attacks.
MAX_GRPC_MESSAGE_SIZE = 128 * 1024 * 1024  # 128 MB in bytes


class MpmGenericRpcHandler(grpc.GenericRpcHandler):
    """A single gRPC GenericRpcHandler that dispatches all RPC calls at call time.

    This is the gRPC equivalent of mprpc's getattr()-based dispatch: instead of
    capturing bound method references once at server startup (as the generated
    add_MpmServerServiceServicer_to_server() does), this handler looks up the
    handler in MPMServer._dynamic_method_registry on every call.

    This is critical for devices like E31x where daughterboard RPC methods are
    registered during claim() → _init_normal() → init_dboards(), which happens
    long after the gRPC server has started.
    """

    SERVICE_NAME = "mpm_server.MpmServerService"

    def __init__(self, mpm_server):
        self._server = mpm_server
        # Build a per-method table of (request_deserializer, response_serializer)
        # from the protobuf descriptor once at startup. This is purely static
        # codec info and does NOT capture any handler function references.
        self._method_codecs = {}
        try:
            service_descriptor = mpm_server__pb2.DESCRIPTOR.services_by_name["MpmServerService"]
        except KeyError as ex:
            err_msg = f"'MpmServerService' not found in protobuf descriptor: {ex}"
            self._server.log.error(err_msg)
            raise RuntimeError(err_msg) from ex
        for method_descriptor in service_descriptor.methods:
            name = method_descriptor.name
            req_class_name = method_descriptor.input_type.name
            resp_class_name = method_descriptor.output_type.name
            req_class = getattr(mpm_server__pb2, req_class_name, None)
            resp_class = getattr(mpm_server__pb2, resp_class_name, None)
            if req_class and resp_class:
                self._method_codecs[name] = (
                    req_class.FromString,
                    resp_class.SerializeToString,
                )
            else:
                missing = ", ".join(
                    t
                    for t, c in ((req_class_name, req_class), (resp_class_name, resp_class))
                    if c is None
                )
                self._server.log.warning(
                    "Skipping codec for '%s': pb2 message type(s) not found: %s", name, missing
                )

    def service_name(self):
        """Return the fully-qualified protobuf service name."""
        return self.SERVICE_NAME

    def service(self, handler_call_details):
        """Look up and return an RPC method handler for the incoming call.

        Called by the gRPC framework on every incoming RPC. The method name is
        extracted from the full path (e.g. '/mpm_server.MpmServerService/Ping'
        → 'Ping') and used to look up the current handler in the registry.
        """
        # Full path is '/ServiceName/MethodName' — validate before splitting
        rpc_path = handler_call_details.method
        if not rpc_path or "/" not in rpc_path:
            self._server.log.warning("Received malformed RPC path: %r", rpc_path)
            return None  # gRPC will respond with UNIMPLEMENTED
        method_name = rpc_path.rsplit("/", 1)[-1]
        if not method_name:
            self._server.log.warning("Empty method name in RPC path: %r", rpc_path)
            return None
        handler_fn = self._server._get_rpc_handler(method_name)
        if handler_fn is None:
            return None  # gRPC will respond with UNIMPLEMENTED
        codecs = self._method_codecs.get(method_name)
        if codecs is None:
            self._server.log.warning(
                "Handler found for '%s' but no codec — proto schema mismatch?", method_name
            )
            return None
        request_deserializer, response_serializer = codecs
        return grpc.unary_unary_rpc_method_handler(
            handler_fn,
            request_deserializer=request_deserializer,
            response_serializer=response_serializer,
        )


class MPMServer(mpm_server_pb2_grpc.MpmServerServiceServicer):
    """Main MPM RPC class.

    This class holds the periph_manager object and translates
    RPC calls to appropiate calls in the periph_manager and dboard_managers.
    """

    ###########################################################################
    # RPC Server Initialization
    ###########################################################################
    def __init__(self, state, default_args):
        """Create a new RPC server.

        :param state: The shared state object
        :param default_args: The arguments passed by the app.
        """
        self.log = get_main_logger().getChild("RPCServer")
        self.log.trace(
            "Launching RPC server with compat num %d.%d", MPM_COMPAT_NUM[0], MPM_COMPAT_NUM[1]
        )
        self._state = state
        self._timer = Greenlet()
        self._timer_lock = RLock()
        # Setting this to True will disable an unclaim on timeout. Use with
        # care, and make sure to set it to False again when finished.
        self._disable_timeouts = False
        self._timeout_interval = float(default_args.get("rpc_timeout_interval", TIMEOUT_INTERVAL))
        self.session_id = None
        # Thread-local storage for per-request client information
        self._client_info = threading.local()
        self._client_info.host = "unknown"
        self._client_info.port = 0
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
        self._state.dev_type.value = to_binary_str(device_info.get("type", "n/a"))
        self._state.dev_product.value = to_binary_str(device_info.get("product", "n/a"))
        self._state.dev_serial.value = to_binary_str(device_info.get("serial", "n/a"))
        self._state.dev_name.value = to_binary_str(device_info.get("name", "n/a"))
        self._state.dev_fpga_type.value = to_binary_str(device_info.get("fpga", "n/a"))
        self._state.dev_customizable_fpga.value = to_binary_str(
            str(device_info.get("customizable_fpga", "n/a"))
        )
        self._db_methods = []
        self._mb_methods = []
        self._server_methods = []
        # Registry for dynamically registered methods (e.g., daughterboard methods added during claim)
        self._dynamic_method_registry = {}
        # Lock protecting _dynamic_method_registry
        self._registry_lock = threading.Lock()
        # Set of RPC names (CamelCase) that are custom server implementations on MPMServer
        self._custom_server_methods = set()
        self.claimed_methods = []
        # Auto-generate gRPC wrappers for server management methods
        self._init_server_rpc_calls()
        # Auto-generate gRPC wrappers for periph_manager methods
        self._init_rpc_calls(self.periph_manager)
        self._state.system_ready.value = True
        self.log.info("RPC server ready!")
        # Optionally spawn watchdog. Note: In order for us to be able to spawn
        # the task from this thread, the main process needs to hand control to
        # us using watchdog.transfer_control().
        if watchdog.has_watchdog():
            self.log.info("Spawning watchdog task...")
            watchdog.spawn_watchdog_task(self._state, self.log)

    @property
    def client_host(self):
        """Get the client host for the current request (thread-local)."""
        return getattr(self._client_info, "host", "unknown")

    @property
    def client_port(self):
        """Get the client port for the current request (thread-local)."""
        return getattr(self._client_info, "port", 0)

    def _extract_client_info(self, context):
        """Extract and store client info from gRPC context in thread-local storage.

        Args:
            context: gRPC context object
        """
        try:
            peer = context.peer()  # e.g., "ipv4:127.0.0.1:54321" or "ipv6:[::1]:54321"
            if not peer:
                self._client_info.host = "unknown"
                self._client_info.port = 0
                return

            if peer.startswith("ipv4:"):
                # IPv4 format: "ipv4:host:port"
                parts = peer[5:].rsplit(":", 1)
                self._client_info.host = parts[0]
                self._client_info.port = int(parts[1]) if len(parts) > 1 else 0

            elif peer.startswith("ipv6:"):
                # IPv6 format: "ipv6:[::1]:port"
                if "[" in peer and "]" in peer:
                    host_port = peer[5:]  # Remove "ipv6:"
                    self._client_info.host = host_port.split("]")[0][1:]  # Extract from [host]
                    port_str = host_port.split("]:")[-1] if "]:" in host_port else "0"
                    self._client_info.port = int(port_str)
            else:
                self._client_info.host = "unknown"
                self._client_info.port = 0

        except (ValueError, IndexError, AttributeError) as ex:
            self.log.trace(
                "Failed to extract client info from peer: %s (%s)",
                peer if "peer" in locals() else "N/A",
                str(ex),
            )
            self._client_info.host = "unknown"
            self._client_info.port = 0
        except Exception as ex:
            self.log.warning("Unexpected error extracting client info: %s", str(ex))
            self._client_info.host = "unknown"
            self._client_info.port = 0

    def _init_server_rpc_calls(self):
        """Register all RPC calls for server management methods.

        Auto-generates gRPC wrappers for server methods like ping, claim,
        unclaim, get_device_info, etc. by introspecting the MPMServer class.
        """
        self._server_methods = []
        self._custom_server_methods = set()
        # Register server methods:
        self._update_component_commands(self, "", "_server_methods")
        # Record which RPC names are custom server implementations.
        # This set is stable for the lifetime of the server and is used by
        # _update_component_commands to prevent MB/DB methods from overriding
        # server methods, independently of the registry state.
        self._custom_server_methods = set(self._server_methods)
        self.log.debug(
            "Registered %d server methods.",
            len(self._server_methods),
        )

    def _init_rpc_calls(self, mgr):
        """Register all RPC calls for the motherboard and daughterboards.

        First clears out all previously registered RPC calls.
        """
        # Clear old calls:
        with self._registry_lock:
            for meth_list in (self._db_methods, self._mb_methods):
                for method in meth_list:
                    # Remove from dynamic registry so stale handlers aren't callable after unclaim/reset
                    self._dynamic_method_registry.pop(method, None)
        self._db_methods = []
        self._mb_methods = []
        # Register new ones:
        self._update_component_commands(mgr, "", "_mb_methods")
        for db_slot, dboard in enumerate(mgr.dboards):
            cmd_prefix = "db_" + str(db_slot) + "_"
            self._update_component_commands(dboard, cmd_prefix, "_db_methods")
        self.log.debug(
            "Registered %d motherboard methods, %d daughterboard methods.",
            len(self._mb_methods),
            len(self._db_methods),
        )

    def _unpack_args_check_token(self, rpc_command, args, context):
        """Unpack arguments and check token validity.

        Analyze the arguments passed to an RPC command, ensuring that a valid
        token is present if required. If the token is missing or invalid, abort
        the RPC call with an UNAUTHENTICATED status.

        Note: context.abort() raises an exception, so this function may not return.
        """
        if "token" not in args:
            context.abort(grpc.StatusCode.UNAUTHENTICATED, "Missing token!")
        token = args.pop("token")
        if not self._check_token_valid(token):
            self.log.trace(
                f"Thwarted attempt to access function `{rpc_command}' with invalid token."
            )
            context.abort(grpc.StatusCode.UNAUTHENTICATED, "Invalid token")
        return args

    def _update_component_commands(self, component, namespace, storage):
        """
        Detect available methods for an object and add them to the RPC server.

        We assume that the object has methods in snake case (PEP8 style), and
        that the RPC calls are CamelCase. So if 'component' has a method called
        'set_frequency()', then there will now be an RPC call SetFrequency.

        This RPC call has to match what is available in the protobuf definition,
        or there will be an error.

        If the method on 'component' requires a claim, then the incoming RPC
        request must include a valid token. The wrapper validates the token and
        strips it before calling the actual method implementation.

        We skip all private methods, and all methods that use the @no_rpc
        decorator.
        """
        available_methods = []

        for method_name in dir(component):
            # Skip private methods, non-callable attributes, and @no_rpc decorated methods
            if (
                method_name.startswith("_")
                or not callable(getattr(component, method_name))
                or getattr(getattr(component, method_name), "_norpc", False)
            ):
                continue

            # For server methods (component is self), only process snake_case methods.
            # This skips the CamelCase methods inherited from MpmServerServiceServicer
            # (e.g. Claim, Unclaim) which would otherwise duplicate the snake_case
            # implementations (claim, unclaim) that we actually want to register.
            if component is self:
                if "_" not in method_name and any(c.isupper() for c in method_name):
                    self.log.trace(
                        f"Skipping CamelCase method {method_name} (inherited gRPC servicer stub)"
                    )
                    continue

            new_rpc_method = getattr(component, method_name)
            new_rpc_pubname = pbu.snake_to_camelcase(method_name)

            # Check if corresponding protobuf types exist
            response_type_name = new_rpc_pubname + "Response"
            request_type_name = new_rpc_pubname + "Request"

            try:
                new_rpc_response_type = getattr(mpm_server__pb2, response_type_name)
                # Verify request type exists as well (raises AttributeError if missing).
                getattr(mpm_server__pb2, request_type_name)

                self.log.trace(f"Binding method call {method_name} to RPC call {new_rpc_pubname}")

                requires_claim = not getattr(new_rpc_method, "_notok", False)

                # All periph_manager / dboard methods go into the dynamic registry.
                # Server methods (component is self) are registered once during startup
                # and tracked in _custom_server_methods.
                should_override = True
                if component is self and new_rpc_pubname in self._custom_server_methods:
                    self.log.trace(
                        f"Method {new_rpc_pubname} already registered as custom server impl, skipping"
                    )
                    should_override = False

                # For periph_manager or dboard methods, check if this would overwrite
                # an existing server method. Server methods take precedence.
                if component is not self and new_rpc_pubname in self._custom_server_methods:
                    self.log.trace(
                        f"Skipping {method_name} -> {new_rpc_pubname}: "
                        f"Server method with same RPC name already registered"
                    )
                    should_override = False

                if should_override:
                    # Determine if this method needs index-based dispatching
                    # This generic mechanism can be extended for other indexed components
                    index_param = None
                    if namespace.startswith("db_") and hasattr(self, "periph_manager"):
                        if hasattr(self.periph_manager, "dboards"):
                            # This is a daughterboard method, enable index-based dispatching with 'db_idx'
                            index_param = "db_idx"
                    # Future extension example:
                    # elif namespace.startswith('channel_') and hasattr(self.domain, 'channels'):
                    #     index_param = 'channel_idx'

                    self._add_rpc_command(
                        new_rpc_method,
                        new_rpc_pubname,
                        new_rpc_response_type,
                        requires_claim,
                        index_param,
                        original_method_name=method_name,  # Pass the original snake_case name
                    )

                    if requires_claim:
                        self.claimed_methods.append(new_rpc_pubname)
                    # storage is the name of the attribute, get the actual list
                    getattr(self, storage).append(new_rpc_pubname)
                    available_methods.append(new_rpc_pubname)

            except AttributeError as e:
                self.log.trace(
                    f"Skipping method {method_name}: No corresponding protobuf type {response_type_name} found"
                )
                continue

        self.log.trace(f"Successfully bound {len(available_methods)} methods: {available_methods}")

    def _add_rpc_command(
        self,
        function,
        rpc_command,
        response_type,
        require_claim,
        index_param=None,
        original_method_name=None,
    ):
        """
        Create a function object for an RPC call and add it as a method to self.
        This enhanced version handles nested types, complex return values, and
        automatic type conversion between Python and protobuf.

        Arguments:
            function: This is the actual function object to be called.
            rpc_command: The RPC command name (in CamelCase).
            response_type: The type of the return value
            require_claim: If true, then the first argument to the RPC call is
                           a token. The function object will check the token
                           first, then strip it from the list of arguments and
                           pass the remaining arguments to 'function'.
            index_param: Optional parameter name used for indexing into self.domain.dboards.
                        If provided, the parameter will be used to dispatch to the correct
                        daughterboard instance. If None, no special dispatching occurs.
            original_method_name: The original snake_case method name, used for dispatching
                        to dboard instances. If None, uses function.__name__.
        """
        self.log.trace(
            f"Adding {'claimed' if require_claim else 'unclaimed'} command "
            f"{rpc_command} pointing to {function}"
        )

        # Store the original function name for daughterboard dispatching
        # Use the provided method name if available, otherwise fall back to function.__name__
        original_function_name = original_method_name or function.__name__

        # Get function signature for better argument handling
        try:
            sig = inspect.signature(function)
            param_names = list(sig.parameters.keys())
        except (ValueError, TypeError):
            param_names = []

        def new_rpc_call(request, context):
            """Define a proxy function for the RPC call with enhanced type handling"""
            try:
                self.log.trace(
                    f"[RPC-WRAPPER] Received {rpc_command} RPC call, require_claim={require_claim}"
                )
                # Extract client info and store in thread-local storage
                self._extract_client_info(context)

                # Unpack arguments from request
                # Use DESCRIPTOR.fields to get ALL fields, not just ListFields() which skips default values
                unpacked_args = {}
                for field_descriptor in request.DESCRIPTOR.fields:
                    field_name = field_descriptor.name
                    # Get the value using getattr (returns default if not set)
                    value = getattr(request, field_name)
                    try:
                        python_value = pbu.protobuf_to_python(value)
                    except ValueError as ex:
                        self.log.error(str(ex))
                        raise
                    unpacked_args[field_name] = python_value

                # Handle token validation if required
                if require_claim:
                    self.log.debug(f"[RPC-WRAPPER] {rpc_command} requires claim, checking token...")
                    unpacked_args = self._unpack_args_check_token(
                        rpc_command, unpacked_args, context
                    )
                    # Because we can only reach this point with a valid claim,
                    # there's no harm in resetting the timer
                    self._reset_timer()

                # Generic handling for index-based dispatching to daughterboard instances
                if index_param and index_param in unpacked_args:
                    index_value = unpacked_args.pop(index_param)  # Remove index param from args

                    # Validate daughterboard list exists
                    if (
                        not hasattr(self.periph_manager, "dboards")
                        or not self.periph_manager.dboards
                    ):
                        err_msg = f"No daughterboards available for {rpc_command}"
                        self.log.error(err_msg)
                        context.abort(grpc.StatusCode.FAILED_PRECONDITION, err_msg)

                    # Validate index bounds
                    if not isinstance(index_value, int):
                        err_msg = f"Invalid {index_param} type: expected int, got {type(index_value).__name__}"
                        self.log.error(err_msg)
                        context.abort(grpc.StatusCode.INVALID_ARGUMENT, err_msg)

                    if index_value < 0 or index_value >= len(self.periph_manager.dboards):
                        err_msg = f"Invalid {index_param}: {index_value} (valid range: 0-{len(self.periph_manager.dboards)-1})"
                        self.log.error(err_msg)
                        context.abort(
                            grpc.StatusCode.INVALID_ARGUMENT,
                            err_msg,
                        )

                    # Override the function to call the specific daughterboard instance
                    target_dboard = self.periph_manager.dboards[index_value]

                    # Check if method exists on daughterboard
                    if not hasattr(target_dboard, original_function_name):
                        err_msg = (
                            f"Method '{original_function_name}' not available on daughterboard {index_value} "
                            f"(type: {type(target_dboard).__name__})"
                        )
                        self.log.error(err_msg)
                        context.abort(grpc.StatusCode.UNIMPLEMENTED, err_msg)

                    actual_function = getattr(target_dboard, original_function_name)
                else:
                    actual_function = function

                # Filter arguments to match function signature
                # Re-inspect the actual function signature if it was changed for indexed dispatching
                sig_to_use = sig  # Default to original signature
                if index_param and "actual_function" in locals():
                    try:
                        actual_sig = inspect.signature(actual_function)
                        actual_param_names = list(actual_sig.parameters.keys())
                        sig_to_use = actual_sig  # Use the new signature
                    except (ValueError, TypeError):
                        actual_param_names = param_names
                else:
                    actual_param_names = param_names

                # Check if function has variadic args (*args) BEFORE filtering
                has_var_positional = False
                if sig_to_use:
                    has_var_positional = any(
                        param.kind == inspect.Parameter.VAR_POSITIONAL
                        for param in sig_to_use.parameters.values()
                    )

                if actual_param_names and not has_var_positional:
                    # Only pass arguments that the function expects
                    filtered_args = {
                        k: v for k, v in unpacked_args.items() if k in actual_param_names
                    }

                    # Debug logging
                    self.log.trace(
                        f"[RPC-WRAPPER] {rpc_command}: unpacked_args keys = {list(unpacked_args.keys())}"
                    )
                    self.log.trace(
                        f"[RPC-WRAPPER] {rpc_command}: actual_param_names = {actual_param_names}"
                    )
                    self.log.trace(
                        f"[RPC-WRAPPER] {rpc_command}: filtered_args keys = {list(filtered_args.keys())}"
                    )

                    # Handle missing arguments for repeated fields (default to empty list)
                    for param_name in actual_param_names:
                        if param_name not in filtered_args:
                            # Check if this corresponds to a repeated field in the request
                            request_descriptor = request.DESCRIPTOR
                            for field in request_descriptor.fields:
                                if field.name == param_name and field.label == field.LABEL_REPEATED:
                                    filtered_args[param_name] = []
                                    break
                elif has_var_positional:
                    # For variadic functions, keep all args except token
                    filtered_args = {k: v for k, v in unpacked_args.items() if k != "token"}
                    self.log.trace(
                        f"[RPC-WRAPPER] {rpc_command}: Variadic function detected, passing all args as positional"
                    )
                    self.log.trace(
                        f"[RPC-WRAPPER] {rpc_command}: filtered_args keys = {list(filtered_args.keys())}"
                    )
                else:
                    # If we can't inspect the function, pass all args except token
                    # (token is only for validation, not a parameter unless explicitly in signature)
                    filtered_args = {k: v for k, v in unpacked_args.items() if k != "token"}

                # Call the actual function
                if has_var_positional:
                    # For variadic functions, always use positional args
                    positional_args = list(filtered_args.values())
                    self.log.trace(
                        f"[RPC-WRAPPER] {rpc_command}: calling variadic with positional args: {positional_args}"
                    )
                    ret_val = actual_function(*positional_args)
                else:
                    # Validate argument binding before calling the function to avoid
                    # catching TypeError exceptions that originate from within the function itself
                    use_positional = False
                    if sig_to_use and actual_param_names:
                        try:
                            # Try binding with keyword arguments first
                            sig_to_use.bind(**filtered_args)
                        except TypeError:
                            # Binding failed with keyword args, try positional (common with C++ bindings)
                            try:
                                positional_args = [
                                    filtered_args[param]
                                    for param in actual_param_names
                                    if param in filtered_args
                                ]
                                sig_to_use.bind(*positional_args)
                                use_positional = True
                                self.log.trace(
                                    f"[RPC-WRAPPER] {rpc_command}: keyword binding failed, will use positional args"
                                )
                            except (TypeError, KeyError) as bind_err:
                                # Both keyword and positional binding failed
                                err_msg = (
                                    f"Argument binding failed for {rpc_command}: {str(bind_err)}"
                                )
                                self.log.error(
                                    f"{err_msg}\nProvided args: {list(filtered_args.keys())}\n"
                                    f"Expected params: {actual_param_names}"
                                )
                                context.abort(grpc.StatusCode.INVALID_ARGUMENT, err_msg)

                    # Now call the function with the validated argument style
                    # TypeError from the actual function execution will propagate correctly
                    if use_positional:
                        ret_val = actual_function(*positional_args)
                    else:
                        ret_val = actual_function(**filtered_args)

                # Build and return the response
                try:
                    return pbu.build_response_from_return_value(ret_val, response_type, self.log)
                except (ValueError, TypeError, AttributeError) as ex:
                    # Response building failed - return value doesn't match schema
                    err_msg = (
                        f"Failed to build response for {rpc_command}: {str(ex)}\n"
                        f"Return value type: {type(ret_val).__name__}\n"
                        f"Return value: {ret_val}"
                    )
                    self.log.error(err_msg)
                    context.abort(grpc.StatusCode.INTERNAL, err_msg)

            except ValueError as ex:
                # Handle domain-specific validation errors
                err_msg = f"Invalid argument in {rpc_command}: {str(ex)}"
                self.log.error(err_msg)
                context.abort(grpc.StatusCode.INVALID_ARGUMENT, err_msg)
            except KeyError as ex:
                # Handle missing required arguments
                err_msg = f"Missing required argument in {rpc_command}: {str(ex)}"
                self.log.error(err_msg)
                context.abort(grpc.StatusCode.INVALID_ARGUMENT, err_msg)
            except AttributeError as ex:
                # Handle attribute access errors (e.g., method not found, missing attribute)
                err_msg = f"Attribute error in {rpc_command}: {str(ex)}"
                self.log.error(err_msg + f"\n{traceback.format_exc()}")
                context.abort(grpc.StatusCode.INTERNAL, err_msg)
            except RuntimeError as ex:
                # Handle runtime errors from device operations
                err_msg = f"Runtime error in {rpc_command}: {str(ex)}"
                self.log.error(err_msg)
                context.abort(grpc.StatusCode.INTERNAL, err_msg)
            except Exception as ex:
                # Handle context.abort() exceptions (expected) and other errors
                if "abort" in str(type(ex).__name__).lower() or str(ex) == "":
                    # This is likely from context.abort(), which is expected behavior
                    # Don't log as an error since it's intentional
                    raise ex
                else:
                    # Handle all other unexpected errors
                    err_msg = (
                        f"[ERROR] Uncaught exception in method {rpc_command}: "
                        f"{str(ex)}\n{traceback.format_exc()}"
                    )
                    self.log.trace(err_msg)
                    context.abort(grpc.StatusCode.INTERNAL, err_msg)

        new_rpc_call.__doc__ = getattr(function, "__doc__", None)

        # All methods go into the dynamic registry for consistent dispatch
        # This allows methods registered after gRPC server startup to be callable
        with self._registry_lock:
            self._dynamic_method_registry[rpc_command] = new_rpc_call
        self.log.trace(f"Registered {rpc_command} in dynamic method registry")

    ###########################################################################
    # Diagnostics and introspection
    ###########################################################################
    def _get_rpc_handler(self, method_name):
        """Return the registered RPC handler for the given method name, or None.

        Used by MpmGenericRpcHandler to look up handlers at call time, avoiding
        direct access to _dynamic_method_registry.
        """
        with self._registry_lock:
            return self._dynamic_method_registry.get(method_name)

    @no_claim
    def list_methods(self):
        """Returns a list of methods.

        For gRPC, returns list of dicts that map to MethodInfo protobuf messages.
        :return: List of dicts with keys: method_name, docstring, requires_claim

        """
        # Use _dynamic_method_registry as the authoritative source of all
        # registered RPC methods (server, MB, and DB methods all end up there).
        # Fall back to the handler's __doc__ where available.
        with self._registry_lock:
            registry_snapshot = list(self._dynamic_method_registry.items())
        return [
            {
                "method_name": rpc_name,
                "docstring": getattr(handler_fn, "__doc__", None) or "",
                "requires_claim": rpc_name in self.claimed_methods,
            }
            for rpc_name, handler_fn in registry_snapshot
        ]

    @no_claim
    def ping(self, data=None):
        """Take in data as argument and send it back.

        This is a safe method which can be called without a claim on the device.
        """
        self.log.debug("I was pinged from: %s:%s", self.client_host, self.client_port)
        return data

    ###########################################################################
    # Claiming logic
    ###########################################################################
    def _check_token_valid(self, token):
        """Check whether a token is valid.

        :param token: The token to check
        :return: True iff
                - The device is currently claimed
                - The claim token matches the one passed in
        """
        token = to_binary_str(token)
        self.log.trace(
            f"[TOKEN-CHECK] Received token: {token[:4].decode('ascii', errors='replace')}**** "
            f"(length: {len(token)}, type: {type(token).__name__})"
        )
        self.log.trace(
            f"[TOKEN-CHECK] Expected token length: {len(self._state.claim_token.value)}"
            f" (type: {type(self._state.claim_token.value).__name__})"
        )
        self.log.trace(f"[TOKEN-CHECK] Claim status: {self._state.claim_status.value}")
        self.log.trace(
            f"[TOKEN-CHECK] Tokens match: {self._state.claim_token.value == token} "
            f"(token = {token[:4].decode('ascii', errors='replace')}****)"
        )
        return (
            self._state.claim_status.value
            and len(token) == TOKEN_LEN
            and self._state.claim_token.value == token
        )

    def _acquire_or_throw(self, lock, error_msg, timeout=LOCK_ACQ_TIMEOUT):
        """Attempt to acquire a shared lock.

        If the timeout is exceeded, then throw an error.
        """
        acquired = lock.acquire(timeout=LOCK_ACQ_TIMEOUT)
        if not acquired:
            self.log.error(error_msg)
            raise RuntimeError("RPC Server Lock Acquire Timeout: " + error_msg)

    @no_claim
    def claim(self, session_id):
        """Claim device.

        Tries to claim MPM device and provides a human readable session_id.
        The caller must remember this token, and call reclaim() on regular
        intervals in order not to lose the claim.

        Will return a token on success, or raise an Exception on failure.
        """
        error_msg = "Claim timed out acquiring the shared state lock after 1 second."
        self._acquire_or_throw(self._state.lock, error_msg)
        if self._state.claim_status.value:
            error_msg = "Someone tried to claim this device again (From: {})".format(
                self.client_host
            )
            self.log.warning(error_msg)
            self._state.lock.release()
            raise RuntimeError("Double-claim")
        self.log.debug("Claiming from: %s, Session ID: %s", self.client_host, session_id)
        self._state.claim_token.value = bytes(
            "".join(choice(ascii_letters + digits) for _ in range(TOKEN_LEN)), "ascii"
        )
        self._state.claim_status.value = True
        self.periph_manager.claimed = True
        self.periph_manager.claim()
        if self.periph_manager.clear_rpc_registry_on_unclaim:
            self._init_rpc_calls(self.periph_manager)
        token_val = self._state.claim_token.value
        self._state.lock.release()
        self.session_id = session_id + " ({})".format(self.client_host)
        self._reset_timer()
        token_prefix = token_val[:4].decode("ascii", errors="replace")
        self.log.trace(f"Token issued to host: {self.client_host} (token = {token_prefix}****)")
        if _is_connection_local(self.client_host):
            self.periph_manager.set_connection_type("local")
        else:
            self.periph_manager.set_connection_type("remote")
        # Return token as string (decode bytes to string for gRPC)
        return token_val.decode("ascii") if isinstance(token_val, bytes) else token_val

    def reclaim(self):
        """Reclaim a MPM device with a token.

        This operation will fail if the
        device is claimed and the token doesn't match, or if the device is not
        claimed at all.
        """
        if self._state.claim_status.value:
            error_msg = "Reclaim timed out acquiring the shared state lock after 1 second."
            self._acquire_or_throw(self._state.lock, error_msg)
            self._state.lock.release()
            self.log.debug("reclaimed from: %s", self.client_host)
            self._reset_timer()
            return True
        self.log.debug("trying to reclaim unclaimed device from: %s", self.client_host)
        return False

    def _unclaim(self):
        """Unconditional unclaim.

        This method is for internal use only.

        Resets and deinitalizes the periph manager as well.
        """
        error_msg = "Unclaim timed out acquiring the shared state lock after 1 second."
        self._acquire_or_throw(self._state.lock, error_msg)
        self.log.debug(
            "Deinitializing device and releasing claim on session `{}'".format(self.session_id)
        )
        # Disable unclaim timer, we're now finished with reclaim loops.
        error_msg = "Unclaim timed out acquiring the timer lock after 1 second."
        self._acquire_or_throw(self._timer_lock, error_msg)
        self._timer.kill(block=False)
        self._timer_lock.release()
        # Now unclaim and deinit the device. We will try and catch any exception
        # here, because the session is over and we have nowhere to send the
        # exception.
        try:
            self.periph_manager.claimed = False
            self.periph_manager.unclaim()
            self.periph_manager.set_connection_type(None)
            self.periph_manager.deinit()
        except BaseException as ex:
            self.log.error("Deinitialization failed: %s", str(ex))
            # Don't want to propagate this failure -- the session is over
        finally:
            # The finally clause is not strictly necessary, because we're catching
            # everything and not returning, but it should be explicit that we
            # must always clear the claim and the _state lock at this point.
            self._state.claim_status.value = False
            self._state.claim_token.value = b""
            self._state.lock.release()
            self.session_id = None

    def unclaim(self):
        """Unclaim token.

        Unclaims the MPM device if it is claimed with this token.
        """
        self._unclaim()
        return True

    def _timeout_event(self):
        "Callback for the claim timeout."
        if self._disable_timeouts:
            self.log.debug("Timeouts are disabled: Snoozing")
            self._reset_timer()
        else:
            self.log.warning("A timeout event occured!")
            self._unclaim()

    def _reset_timer(self):
        """Reset unclaim timer.

        After calling this, call this function again
        within 'timeout' seconds to avoid a timeout event.
        """
        error_msg = "Reset Timer timed out acquiring the timer lock after 1 second."
        self._acquire_or_throw(self._timer_lock, error_msg)
        self._timer.kill()
        self._timer = spawn_later(self._timeout_interval, self._timeout_event)
        self._timer_lock.release()

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
    @no_claim
    def get_mpm_compat_number(self):
        """Get the MPM compatibility number.

        :return: current MPM compatibility number.
        """
        return MPM_COMPAT_NUM

    @no_claim
    def get_device_info(self):
        """Get device information.

        This is as safe method which can be called without a claim on the device
        """
        if self.periph_manager is None:
            err_msg = "Device is currently resetting, please retry."
            self.log.warning(err_msg)
            raise RuntimeError(err_msg)
        info = self.periph_manager.get_device_info()
        info["mpm_version"] = "{}.{}".format(*MPM_COMPAT_NUM)
        if "customizable_fpga" in info:
            info["customizable_fpga"] = str(info["customizable_fpga"])
        if _is_connection_local(self.client_host):
            info["connection"] = "local"
        else:
            info["connection"] = "remote"
        return info

    def get_log_buf(self):
        """Return the contents of the log.

        :return: The log buffer as a list of str -> str dictionaries.
        """
        log_records = get_main_logger().get_log_buf()
        self.log.trace("Returning %d log records.", len(log_records))
        return [{k: str(v) for k, v in iter(record.items())} for record in log_records]

    ###########################################################################
    # Session initialization
    ###########################################################################
    def init(self, args):
        """Initialize device.

        See PeriphManagerBase for details. This is forwarded
        from here import to give extra control over the claim release timeout.
        """
        # Disable the claim timeout while init() runs: MCR reconfigurations
        # (e.g. RFDC PLL setup) can easily exceed the 5-second claim timer,
        # which would fire _unclaim() and invalidate the token before init()
        # returns. This is the same pattern used in reset_timer_and_mgr().
        with self._timeout_disabler():
            result = self.periph_manager.init(args)
        self.log.debug("init() result: {}".format(result))
        return result

    ###########################################################################
    # Update components
    ###########################################################################
    def _reset_mgr(self):
        """Reset the Peripheral Manager for this RPC server.

        This is a helper function that resets the peripheral manager.
        The peripheral manager is torn down and re-initialized from scratch.
        """
        self.log.info("Resetting peripheral manager.")
        # Keep a weak reference so we can verify the old manager is truly
        # destroyed after we clear all references. A weak reference does not
        # prevent garbage collection, so if it returns None afterwards we know
        # __del__ was called and all handles were released.
        _old_mgr_ref = weakref.ref(self.periph_manager)
        self.periph_manager.tear_down()
        # Clear bound-method closures AFTER tear_down() to ensure that if
        # tear_down() raises, the RPCs remain registered and the old manager
        # is still reachable. The gRPC implementation stores all handlers only
        # in _dynamic_method_registry (unlike the old mprpc server which also
        # used setattr and a Cython cache).
        with self._registry_lock:
            for method in self._mb_methods + self._db_methods:
                self._dynamic_method_registry.pop(method, None)
        self._mb_methods = []
        self._db_methods = []
        self.periph_manager = None
        if _old_mgr_ref() is not None:
            self.log.warning(
                "Peripheral manager was NOT garbage collected after reset! "
                "There is probably a lingering reference keeping it alive. "
                "GPIO/SPI/pipe handles may have leaked."
            )
        # Create a new manager and register RPC calls for it.
        self.periph_manager = self._mgr_generator()
        self._init_rpc_calls(self.periph_manager)
        # Update the FPGA type information in the state
        device_info = self.periph_manager.get_device_info()
        self._state.dev_fpga_type.value = to_binary_str(device_info.get("fpga", "n/a"))

    def reset_timer_and_mgr(self):
        """Reset timer and periph manager.

        Pause the timers, reset the peripheral manager and restart the timers.
        """
        # Stop the timer, reset_timer_and_mgr can take some time:
        with self._timeout_disabler():
            self._reset_mgr()
            self.log.debug("Reset the periph manager")

        self.log.debug("End of reset_timer_and_mgr")
        self._reset_timer()

    def update_component(self, file_metadata_l, data_l):
        """Updates the device component files specified by the metadata and data.

        :param file_metadata_l: List of dictionary of strings containing metadata
        :param data_l: List of binary string with the file contents to be written
        """
        with self._timeout_disabler():
            result = self.periph_manager.update_component(file_metadata_l, data_l)
            if not result:
                component_ids = [metadata["id"] for metadata in file_metadata_l]
                raise RuntimeError("Failed to update components: {}".format(component_ids))

            # Check if we need to reset the peripheral manager
            reset_now = False
            for metadata, data in zip(file_metadata_l, data_l):
                # Make sure the component is in the updateable_components
                component_id = metadata["id"]
                if component_id in self.periph_manager.updateable_components:
                    # Check if that updating that component means the PM should be reset
                    reset_now = (
                        reset_now
                        or self.periph_manager.updateable_components[component_id]["reset"]
                    ) and not metadata.get("reset", "").lower() == "false"
                else:
                    self.log.debug(
                        "ID {} not in updateable components ({})".format(
                            component_id, self.periph_manager.updateable_components
                        )
                    )
            try:
                self.log.trace("Reset after updating component? {}".format(reset_now))
                if reset_now:
                    self._reset_mgr()
                    self.log.debug("Reset the periph manager")
            except Exception as ex:
                self.log.error("Error in update_component while resetting: {}".format(ex))
                raise

        self.log.debug("End of update_component")
        self._reset_timer()


def _is_connection_local(client_hostname):
    """Check if the connection is local.

    :param client_hostname: The hostname of the client to be checked
    :return: True if the connection is local, False otherwise
    """
    return client_hostname in net.get_local_ip_addrs()


###############################################################################
# Process control
###############################################################################
def _rpc_server_process(shared_state, port, default_args):
    """This is the actual process that's running the RPC server.

    :param shared_state: The shared state object
    :param port: The port to run the RPC server on
    :param default_args: The arguments passed by the app
    """
    set_proc_title(current_process().name, get_main_logger().getChild("RPCServer"))

    # Create gRPC server with MAX_GRPC_MESSAGE_SIZE limit.
    server = grpc.server(
        futures.ThreadPoolExecutor(max_workers=10),
        options=[
            ("grpc.max_send_message_length", MAX_GRPC_MESSAGE_SIZE),
            ("grpc.max_receive_message_length", MAX_GRPC_MESSAGE_SIZE),
        ],
    )

    mpm_server = MPMServer(shared_state, default_args)
    # Use MpmGenericRpcHandler instead of the generated add_*_to_server().
    # The generated function captures bound method references at startup, making
    # methods registered later (e.g. dboard methods during claim()) invisible.
    # MpmGenericRpcHandler dispatches via _dynamic_method_registry at call time,
    # matching the mprpc behaviour of resolving handlers on every request.
    server.add_generic_rpc_handlers([MpmGenericRpcHandler(mpm_server)])

    server.add_insecure_port(f"[::]:{port}")
    server.start()
    get_main_logger().getChild("RPCServer").info("Server started, listening on " + str(port))
    # Catch SIGTERM/SIGINT: set a threading.Event so the signal handler itself
    # never blocks (gevent disallows blocking inside signal callbacks).
    # The main thread waits on the event, then does a best-effort server.stop()
    # followed by os._exit(0).  os._exit() bypasses Python's thread-join loop,
    # which would otherwise hang forever on the gRPC-Core C++ threads
    # (event_engine, lifeguard, grpc_global_tim) that are never collected by
    # the normal Python shutdown sequence.
    stop_event = threading.Event()
    register_chained_signal_handler(signal.SIGTERM, lambda *args: stop_event.set())
    register_chained_signal_handler(signal.SIGINT, lambda *args: stop_event.set())

    try:
        # Block until SIGTERM/SIGINT arrives. server.wait_for_termination() is
        # NOT used here because under gevent + init_grpc_gevent() it returns as
        # soon as the active connection closes, causing the server to stop
        # accepting new connections after every clean client disconnect.
        stop_event.wait()
    except (KeyboardInterrupt, SystemExit):
        pass
    finally:
        server.stop(grace=0)
        # Tear down explicitly so child processes (e.g. DIO fault monitors) are
        # terminated before os._exit() bypasses Python's multiprocessing cleanup.
        if mpm_server.periph_manager is not None:
            try:
                mpm_server.periph_manager.tear_down()
            except Exception as ex:  # pylint: disable=broad-except
                get_main_logger().getChild("RPCServer").warning(
                    "Error during periph manager teardown at shutdown: %s", ex
                )
        # Force-kill the process so gRPC-Core C++ threads don't prevent exit.
        os._exit(0)


def spawn_rpc_process(state, udp_port, default_args):
    """Returns a process that contains the RPC server.

    :param state: The shared state object
    :param port: The port to run the RPC server on
    :param default_args: The arguments passed by the app
    """
    proc = Process(
        target=_rpc_server_process,
        name="RPCServer",
        args=[state, udp_port, default_args],
    )
    proc.start()
    return proc
