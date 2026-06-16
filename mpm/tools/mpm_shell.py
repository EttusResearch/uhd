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

import argparse
import cmd
import multiprocessing
import os
import re
import sys
import time
from importlib import import_module

import grpc

try:
    from usrp_mpm.mpmtypes import MPM_RPC_PORT
except ImportError:
    MPM_RPC_PORT = None

# Import protobuf modules from the installed package path.
try:
    from usrp_mpm import mpm_server_pb2, mpm_server_pb2_grpc
except ImportError as e:
    print("Error: Could not import usrp_mpm protobuf modules.")
    print(
        "Info: For source-tree development, set PYTHONPATH to include "
        "your build Python output directory."
    )
    print(f"Original import error: {e}")
    sys.exit(1)


def _field_is_repeated(field):
    """Check if a protobuf field is repeated, compatible with protobuf 3.x and 4.x+."""
    try:
        return field.is_repeated  # protobuf 3.x
    except AttributeError:
        return field.label == field.LABEL_REPEATED  # protobuf 4.x+ (UPB backend)


def _camel_to_snake(name):
    """Convert a CamelCase name to snake_case."""
    s1 = re.sub(r"(.)([A-Z][a-z]+)", r"\1_\2", name)
    return re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", s1).lower()


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
        "host",
        help="Specify host to connect to.",
        default=None,
    )
    parser.add_argument(
        "-p",
        "--port",
        type=int,
        help="Specify port to connect to.",
        default=MPM_RPC_PORT,
    )
    parser.add_argument("-c", "--claim", action="store_true", help="Claim device after connecting.")
    parser.add_argument(
        "-j", "--hijack", type=str, help="Hijack running session (excludes --claim)."
    )
    parser.add_argument(
        "-s",
        "--script",
        type=str,
        help="Run shell in scripting mode. Specified script contains "
        "MPM shell commands, one per line.",
    )
    return parser.parse_args()


def split_args(args, *default_args):
    "Returns an array of args, space-separated"
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
            args=(host, port, self._cmd_q, self._token_q),
        )
        self._claim_loop.start()

    def claim_loop(self, host, port, cmd_q, token_q):
        """
        Run a claim loop.
        """
        command = None
        token = None
        exit_loop = False

        # Create gRPC channel and stub
        channel = grpc.insecure_channel(f"{host}:{port}")
        stub = mpm_server_pb2_grpc.MpmServerServiceStub(channel)

        try:
            while not exit_loop:
                if token and not command:
                    # Reclaim with existing token
                    try:
                        request = mpm_server_pb2.ReclaimRequest(token=token)
                        response = stub.Reclaim(request)
                        if not response.success:
                            print("Reclaim failed, token may have expired")
                            token = None
                            token_q.put(None)
                    except grpc.RpcError as e:
                        print(f"RPC error during reclaim: {e}")
                        token = None
                        token_q.put(None)

                elif command == "claim":
                    if not token:
                        try:
                            request = mpm_server_pb2.ClaimRequest(session_id="MPM Shell")
                            response = stub.Claim(request)
                            token = response.token
                        except grpc.RpcError as e:
                            print(f"Failed to claim: {e}")
                            token = None
                    else:
                        print("Already have claim")
                    token_q.put(token)

                elif command == "unclaim":
                    if token:
                        try:
                            request = mpm_server_pb2.UnclaimRequest(token=token)
                            stub.Unclaim(request)
                        except grpc.RpcError as e:
                            print(f"Failed to unclaim: {e}")
                    token = None
                    token_q.put(None)

                elif command == "exit":
                    if token:
                        try:
                            request = mpm_server_pb2.UnclaimRequest(token=token)
                            stub.Unclaim(request)
                        except grpc.RpcError as e:
                            print(f"Failed to unclaim on exit: {e}")
                    token = None
                    token_q.put(None)
                    exit_loop = True

                time.sleep(1)
                command = None
                if not cmd_q.empty():
                    command = cmd_q.get(False)

        except Exception as ex:
            print("Unexpected error in claimer loop!")
            print(str(ex))
        finally:
            channel.close()

    def exit(self):
        """
        Unclaim device and exit claim loop.
        """
        self.unclaim()
        self._cmd_q.put("exit")
        self._claim_loop.join()

    def unclaim(self):
        """
        Unclaim device.
        """
        if not self.hijacked:
            self._cmd_q.put("unclaim")
        else:
            self.hijacked = False
        self.token = None

    def claim(self):
        """
        Claim device.
        """
        self._cmd_q.put("claim")
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
        self.channel = None
        self.stub = None
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
        snake_name = _camel_to_snake(command)
        cmd_name = "do_" + snake_name
        if not hasattr(self, cmd_name):
            new_command = lambda args: self.rpc_template(str(command), requires_token, args)
            new_command.__doc__ = docs
            setattr(self, cmd_name, new_command)
            self.remote_methods.append(snake_name)

    def _print_response(self, response):
        print(re.sub("^", "< ", response, flags=re.MULTILINE))

    def _create_request_message(self, method_name, args, requires_token):
        """
        Dynamically create a protobuf request message for a given method.
        """
        # method_name is already in CamelCase from ListMethods
        # Get the request message type
        request_class_name = f"{method_name}Request"
        try:
            request_class = getattr(mpm_server_pb2, request_class_name)
        except AttributeError:
            raise ValueError(f"Unknown request type: {request_class_name}")

        # Create request object
        request = request_class()

        # Add token if required
        if requires_token:
            token = self._claimer.get_token()
            if token and hasattr(request, "token"):
                request.token = token

        # Parse and add other arguments
        if args:
            expanded_args = self.expand_args(args)

            # Get field names from the request descriptor
            field_names = [
                field.name for field in request.DESCRIPTOR.fields if field.name != "token"
            ]

            # Assign args to fields
            for i, (field_name, arg_value) in enumerate(zip(field_names, expanded_args)):
                field = request.DESCRIPTOR.fields_by_name.get(field_name)
                if field:
                    from google.protobuf import descriptor

                    is_string_map = field.message_type and field.message_type.name == "StringMap"

                    # Handle repeated fields first (before any message-type inspection,
                    # since getattr() returns a container for repeated fields)
                    if _field_is_repeated(field) and is_string_map:
                        # repeated StringMap (e.g., UpdateComponentRequest.file_metadata_l)
                        repeated_field = getattr(request, field_name)
                        entries = arg_value if isinstance(arg_value, list) else [arg_value]
                        for entry in entries:
                            if isinstance(entry, dict):
                                new_entry = repeated_field.add()
                                new_entry.data.update(entry)
                    elif _field_is_repeated(field):
                        # repeated primitive/other message
                        repeated_field = getattr(request, field_name)
                        entries = arg_value if isinstance(arg_value, list) else [arg_value]
                        repeated_field.extend(entries)
                    elif is_string_map:
                        # singular StringMap
                        if isinstance(arg_value, dict):
                            getattr(request, field_name).data.update(arg_value)
                    else:
                        # Convert arg_value to appropriate type based on field type
                        if field.type == descriptor.FieldDescriptor.TYPE_INT32:
                            arg_value = int(arg_value)
                        elif field.type == descriptor.FieldDescriptor.TYPE_INT64:
                            arg_value = int(arg_value)
                        elif field.type == descriptor.FieldDescriptor.TYPE_UINT32:
                            arg_value = int(arg_value)
                        elif field.type == descriptor.FieldDescriptor.TYPE_UINT64:
                            arg_value = int(arg_value)
                        elif field.type == descriptor.FieldDescriptor.TYPE_DOUBLE:
                            arg_value = float(arg_value)
                        elif field.type == descriptor.FieldDescriptor.TYPE_FLOAT:
                            arg_value = float(arg_value)
                        elif field.type == descriptor.FieldDescriptor.TYPE_BOOL:
                            arg_value = (
                                arg_value.lower() in ("true", "1", "yes")
                                if isinstance(arg_value, str)
                                else bool(arg_value)
                            )
                        elif field.type == descriptor.FieldDescriptor.TYPE_STRING:
                            arg_value = str(arg_value)
                        # For other types (message, enum, etc.), leave as-is
                        setattr(request, field_name, arg_value)

        return request

    def _format_response(self, response):
        """Format a protobuf response for display"""
        if response is None:
            return "None"

        # Handle simple responses
        if hasattr(response, "success"):
            return str(response.success)

        # Handle string responses
        if hasattr(response, "data") and isinstance(response.data, str):
            return response.data

        # Handle responses with a single field
        fields = [field for field in response.DESCRIPTOR.fields]
        if len(fields) == 1:
            field = fields[0]
            value = getattr(response, field.name)
            from google.protobuf import descriptor

            # Handle repeated fields first (before any message-type inspection,
            # since value is a container object for repeated fields and does not
            # have attributes like .data that belong to individual messages)
            if _field_is_repeated(field):
                # repeated StringMap -> list of plain dicts
                if field.message_type and field.message_type.name == "StringMap":
                    return str([dict(entry.data) for entry in value])
                return str(list(value))
            # Handle singular StringMap
            elif field.message_type and field.message_type.name == "StringMap":
                return str(dict(value.data))
            else:
                return str(value)

        # For complex responses, return string representation
        return str(response)

    def rpc_template(self, command, requires_token, args=None):
        """
        Template function to create new RPC shell commands.
        """
        if requires_token and (self._claimer is None or self._claimer.get_token() is None):
            self._print_response("Cannot execute `{}' -- " "no claim available!".format(command))
            return False

        try:
            # Create the request message
            request = self._create_request_message(command, args, requires_token)

            # Get the gRPC method from stub (command is already in CamelCase)
            grpc_method = getattr(self.stub, command, None)

            if grpc_method is None:
                self._print_response(f"Method {command} not found on stub")
                return False

            # Call the gRPC method
            response = grpc_method(request)

            # Format and print response
            formatted_response = self._format_response(response)
            # Only treat True/False as a success indicator if the response field
            # is literally named 'success'. All other bool fields (e.g. lo_lock)
            # should print their actual value.
            if hasattr(response, "success") and formatted_response in ["True", "False"]:
                if formatted_response == "True":
                    self._print_response("Command succeeded.")
                else:
                    self._print_response("Command failed!")
            else:
                self._print_response(formatted_response)

        except grpc.RpcError as ex:
            self._print_response("RPC Command failed!\nError: {}".format(ex.details()))
            return False
        except Exception as ex:
            self._print_response("Unexpected exception!\nError: {}".format(ex))
            return False

        return False

    def get_names(self):
        "We need this for tab completion."
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
        # cmd.Cmd uses 'tab: complete' which is GNU readline syntax.
        # Platforms using libedit (e.g. embedded Linux, macOS) need a
        # different binding; re-apply the correct one here.
        try:
            import readline

            if "libedit" in (readline.__doc__ or ""):
                readline.parse_and_bind("bind ^I rl_complete")
        except ImportError:
            pass
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
        print("Attempting to connect to {host}:{port}...".format(host=host, port=port))
        try:
            # Create gRPC channel and stub, matching the message size limits used
            # by mpm_client.cpp (128 MB) to handle large payloads like firmware updates.
            MAX_GRPC_MESSAGE_SIZE = 128 * 1024 * 1024  # 128 MB
            channel_options = [
                ("grpc.max_receive_message_length", MAX_GRPC_MESSAGE_SIZE),
                ("grpc.max_send_message_length", MAX_GRPC_MESSAGE_SIZE),
            ]
            self.channel = grpc.insecure_channel(f"{host}:{port}", options=channel_options)
            self.stub = mpm_server_pb2_grpc.MpmServerServiceStub(self.channel)

            # Test connection with ping
            ping_test_data = "test"
            ping_request = mpm_server_pb2.PingRequest(data=ping_test_data)
            ping_response = self.stub.Ping(ping_request)

            # Verify ping response
            if not hasattr(ping_response, "data"):
                print("Warning: Ping response missing data field")
                print("Connection established but ping verification failed.")
            elif ping_response.data != ping_test_data:
                print(
                    f"Warning: Ping response mismatch. Sent '{ping_test_data}', got '{ping_response.data}'"
                )
                print("Connection established but ping verification failed.")
            else:
                print("Connection successful.")

        except Exception as ex:
            print("Connection refused")
            print("Error: {}".format(ex))
            return False

        self._host = host
        self._port = port

        print("Getting methods...")
        try:
            # Call ListMethods to get available methods
            list_methods_request = mpm_server_pb2.ListMethodsRequest()
            methods_response = self.stub.ListMethods(list_methods_request)

            # Add each method as a command
            for method in methods_response.methods:
                self._add_command(method.method_name, method.docstring, method.requires_claim)
            print("Added {} methods.".format(len(methods_response.methods)))

        except grpc.RpcError as ex:
            print("Failed to get methods list")
            print("Error: {}".format(ex.details()))
            return False

        print("Querying device info...")
        try:
            device_info_request = mpm_server_pb2.GetDeviceInfoRequest()
            device_info_response = self.stub.GetDeviceInfo(device_info_request)
            self._device_info = dict(device_info_response.dev_info.data)
        except grpc.RpcError as ex:
            print("Warning: Could not get device info")
            print("Error: {}".format(ex.details()))
            self._device_info = {}

        return True

    def disconnect(self):
        """
        Clean up after a connection was closed.
        """
        self._device_info = None
        if self._claimer is not None:
            self._claimer.exit()
        if self.channel:
            try:
                self.channel.close()
            except Exception as ex:
                print("Error while closing the connection")
                print("Error: {}".format(ex))
        for method in self.remote_methods:
            delattr(self, "do_" + method)
        self.remote_methods = []
        self.stub = None
        self.channel = None
        self._host = None
        self._port = None

    def claim(self):
        "Initialize claim"
        print("Claiming device...")
        self._claimer.claim()
        return True

    def hijack(self, token):
        "Hijack running session"
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
        if not self._device_info:
            self.prompt = "> "
        else:
            token = self._claimer.get_token()
            if token is None:
                claim_status = ""
            elif self._claimer.hijacked:
                claim_status = " [H]"
            else:
                claim_status = " [C]"
            self.prompt = "{dev_id}{claim_status}> ".format(
                dev_id=self._device_info.get("name", self._device_info.get("serial", "?")),
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
        self.cmdqueue.append("EOF")  # terminate shell after script execution

    def expand_args(self, args):
        """
        Takes a string and returns a list
        """
        if self._claimer is not None and self._claimer.get_token() is not None:
            args = args.replace("$T", str(self._claimer.get_token()))
        eval_preamble = "="
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
        host, port = split_args(args, "localhost", MPM_RPC_PORT)
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
        return True  # orderly shutdown


def main():
    "Go, go, go!"
    args = parse_args()
    my_shell = MPMShell(args.host, args.port, args.claim, args.hijack, args.script)

    try:
        my_shell.cmdloop()
    except KeyboardInterrupt:
        my_shell.disconnect()
    except Exception as ex:  # pylint: disable=broad-except
        print("Uncaught exception: " + str(ex))
        my_shell.disconnect()
        return False
    return True


if __name__ == "__main__":
    exit(not main())
