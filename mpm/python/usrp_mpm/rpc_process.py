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
Code to run the discovery port
"""

from __future__ import print_function
from multiprocessing import Process

from gevent.server import StreamServer
from mprpc import RPCServer
from types import graceful_exit, MPM_RPC_PORT
import rpc_server



def spawn_rpc_process(server, state, udp_port=MPM_RPC_PORT):
    """
    Returns a process that contains the RPC server
    """

    p_args = [server, udp_port, state]
    p = Process(target=_rpc_server_process, args=p_args)
    p.start()
    return p


def _rpc_server_process(server, shared_state, port):
    try:
        rpc_class = getattr(rpc_server, server+"Server")
        server = StreamServer(('0.0.0.0', port), handle=rpc_class(shared_state))
        server.serve_forever()
    except graceful_exit:
        server.close()
