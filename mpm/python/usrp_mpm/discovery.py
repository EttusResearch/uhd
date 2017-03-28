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
import time
import ctypes
from multiprocessing import Process, Value
from six import iteritems
import socket
from types import MPM_DISCOVERY_PORT, graceful_exit

RESPONSE_PREAMBLE = "USRP-MPM"
RESPONSE_SEP = ";"
RESPONSE_CLAIMED_KEY = "claimed"



def spawn_discovery_process(device_info, shared_state):
    """
    Returns a process that contains the device discovery.

    Arguments:
    device_info -- A dictionary of type string -> string. All of these items
                   will be included in the response string.
    """
    # claim_status = Value(ctypes.c_bool, False)
    p = Process(target=_discovery_process, args=(device_info, shared_state))
    p.start()
    return p


def _discovery_process(device_info, state):
    """
    The actual process for device discovery. Is spawned by
    spawn_discovery_process().
    """
    def create_response_string():
        " Generate the string that gets sent back to the requester. "
        return RESPONSE_SEP.join(
            [RESPONSE_PREAMBLE] + \
            ["{k}={v}".format(k=k, v=v) for k, v in iteritems(device_info)] + \
            ["{k}={v}".format(k=RESPONSE_CLAIMED_KEY, v=state.claim_status.value)] + \
            ["{k}={v}".format(k="token", v=state.claim_token.value)]
        )

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((("0.0.0.0", MPM_DISCOVERY_PORT)))

    send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        while True:
            data, sender = sock.recvfrom(4096)
            if data.strip("\0") == "MPM-DISC":
                send_data = create_response_string()
                send_sock.sendto(send_data, sender)
    except:
        sock.close()
        send_sock.close()
