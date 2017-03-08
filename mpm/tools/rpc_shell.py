#!/usr/bin/env python
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
RPC shell to debug USRP MPM capable devices
"""

import sys
import cmd
import types
from functools import partial
from mprpc import RPCClient
from mprpc.exceptions import RPCError
from usrp_mpm import types

def rpc_template(obj, command, args):
    try:
        if args:
            response = obj.client.call(command, args)
        else:
            response = obj.client.call(command)
    except RPCError as e:
        print("RPC Command failed!")
        print("Error: {}".format(e))
        return
    if isinstance(response, bool):
        if response:
            print("Commend executed successfully!")
            return
        print("Command failed!")
        return
    print(response)

class RPCShell(cmd.Cmd):
    prompt="MPM> "
    client = None
    remote_methods = []

    def do_connect(self, host, port=types.MPM_RPC_PORT):
        try:
            self.client = RPCClient(host, port)
        except:
            print("Connection refused")
            return
        methods = self.client.call('list_methods')
        for method in methods:
            self.add_command(*method)

    def do_disconnect(self, args):
        if self.client:
            try:
                self.client.close()
            except RPCError as e:
                print("Error while closing the connection")
                print("Error: {}".format(e))
        for method in self.remote_methods:
            delattr(self, "do_"+method)
        self.remote_methods = []
        self.client = None


    def add_command(self, command, docs):
        new_command = partial(rpc_template, self, str(command))
        new_command.__doc__ = docs
        setattr(self, "do_"+command, new_command)
        self.remote_methods.append(command)

    def run(self):
        try:
            self.cmdloop()
        except KeyboardInterrupt:
            self.do_disconnect(None)
            exit(0)

    def get_names(self):
        return dir(self)


if __name__ == "__main__":
    my_shell = RPCShell()
    exit(not(my_shell.run()))

