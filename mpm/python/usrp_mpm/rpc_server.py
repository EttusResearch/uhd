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
from gevent.server import StreamServer
from types import graceful_exit, MPM_RPC_PORT
from mprpc import RPCServer
from six import iteritems
import time

from multiprocessing import Process

class MPMServer(RPCServer):
    _db_methods = {}
    def __init__(self, state, mgr):
        self._state = state
        # Instead do self.mboard = periphs.init_periph_manager(args...)
        self.periph_manager = mgr
        for db_slot, db in iteritems(mgr.dboards):
            methods = (m for m in dir(db) if not m.startswith('_') and callable(getattr(db, m)))
            for method in methods:
                command_name = 'db_'+ db_slot + '_' + method
                self._add_command(getattr(db,method), command_name)
                db_methods = self._db_methods.get(db_slot, [])
                db_methods.append(command_name)
                self._db_methods.update({db_slot: db_methods})

        # When we do init we can just add dboard/periph_manager methods with setattr(self, method)
        # Maybe using partial
        # To remove methods again we also have to remove them from self._methods dict (they're cached)
        super(MPMServer, self).__init__()

    def _add_command(self, function, command):
        setattr(self, command, function)


    def list_methods(self):
        """
        Returns all public methods of this RPC server
        """
        methods = filter(lambda entry: not entry.startswith('_'), dir(self)) # Return public methods
        methods_with_docs = map(lambda m: (m, getattr(self, m).__doc__), methods)
        return methods_with_docs

    def ping(self, data=None):
        """
        Take in data as argument and send it back
        """
        return data

    def claim(self, token):
        """
        claim `token` - claims the MPM device with given token
        """
        if self._state.claim_status.value:
            if self._state.claim_token.value == token:
                return True
            return False
        self._state.claim_status.value = True
        self._state.claim_token.value = token
        return True

    def unclaim(self, token):
        """
        unclaim `token` - unclaims the MPM device if it is claimed with this token
        """
        if self._state.claim_status.value and self._state.claim_token.value == token:
            self._state.claim_status.value = False
            self._state.claim_token.value = ""
            return True
        return False


def _rpc_server_process(shared_state, port, mgr):
    """
    Start the RPC server
    """
    server = StreamServer(('0.0.0.0', port), handle=MPMServer(shared_state, mgr))
    try:
        server.serve_forever()
    except:
        server.close()


def spawn_rpc_process(state, udp_port, mgr):
    """
    Returns a process that contains the RPC server
    """

    p_args = [udp_port, state, mgr]
    p = Process(target=_rpc_server_process, args=p_args)
    p.start()
    return p
