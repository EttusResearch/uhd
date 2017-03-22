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
from mprpc import RPCServer
from usrp_mpm import periphs


class EchoServer(RPCServer):
    def echo(self, arg):
        print(arg)
        return arg


class ClaimServer(RPCServer):
    def __init__(self, state):
        self._state = state
        super(ClaimServer, self).__init__()

    def claim(self, token):
        'claim `token` - claims the MPM device with given token'
        if self._state.claim_status.value:
            if self._state.claim_token.value == token:
                return True
            return False
        self._state.claim_status.value = True
        self._state.claim_token.value = token
        return True

    def unclaim(self, token):
        'unclaim `token` - unclaims the MPM device if it is claimed with this token'
        if self._state.claim_status.value and self._state.claim_token.value == token:
            self._state.claim_status.value = False
            self._state.claim_token.value = ""
            return True
        return False

    def list_methods(self):
        methods = filter(lambda entry: not entry.startswith('_'), dir(self)) # Return public methods
        methods_with_docs = map(lambda m: (m, getattr(self,m).__doc__), methods)
        return methods_with_docs


class MPMServer(RPCServer):
    def __init__(self, state):
        # Instead do self.mboard = periphs.init_periph_manager(args...)
        self.periph_manager = periphs.init_periph_manager()
        # When we do init we can just add dboard/periph_manager methods with setattr(self, method)
        # Maybe using partial
        # To remove methods again we also have to remove them from self._methods dict (they're cached)

    def get_clock_id(self, dboard):
        dboard = getattr(self.mboard, "get_dboard_"+dboard)
        clk = dboard.get_clock_gen()
        return clk.get_chip_id()
