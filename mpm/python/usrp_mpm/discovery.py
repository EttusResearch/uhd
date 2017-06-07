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
import socket
from builtins import bytes
from six import iteritems
from usrp_mpm.mpmtypes import MPM_DISCOVERY_PORT
from .mpmlog import get_main_logger

RESPONSE_PREAMBLE = "USRP-MPM"
RESPONSE_SEP = ";"
RESPONSE_CLAIMED_KEY = "claimed"

def spawn_discovery_process(device_info, shared_state, discovery_addr):
    """
    Returns a process that contains the device discovery.

    Arguments:
    device_info -- A dictionary of type string -> string. All of these items
                   will be included in the response string.
    shared_state -- Shared state of device (is it claimed, etc.). Is a
                    SharedState() object.
    discovery_addr -- Discovery will listen on this address(es)
    """
    proc = Process(
        target=_discovery_process,
        args=(device_info, shared_state, discovery_addr)
    )
    proc.start()
    return proc


def _discovery_process(device_info, state, discovery_addr):
    """
    The actual process for device discovery. Is spawned by
    spawn_discovery_process().
    """
    def create_response_string():
        " Generate the string that gets sent back to the requester. "
        return RESPONSE_SEP.join(
            [RESPONSE_PREAMBLE] + \
            ["{k}={v}".format(k=k, v=v) for k, v in iteritems(device_info)] + \
            ["{k}={v}".format(k=RESPONSE_CLAIMED_KEY, v=state.claim_status.value)]
        )
    log = get_main_logger().getChild('discovery')

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # FIXME really, we should only bind to the subnet but I haven't gotten that
    # working yet
    sock.bind((("0.0.0.0", MPM_DISCOVERY_PORT)))
    send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # TODO yeah I know that's not how you do this
    discovery_addr_prefix  = discovery_addr.replace('.255', '')
    if discovery_addr == '0.0.0.0':
        discovery_addr_prefix = ''

    try:
        while True:
            data, sender = sock.recvfrom(8000)
            log.info("Got poked by: %s", sender[0])
            # TODO this is still part of the awful subnet identification
            if not sender[0].startswith(discovery_addr_prefix):
                continue
            if data.strip(b"\0") == b"MPM-DISC":
                log.info("Sending discovery response to %s port: %d",
                         sender[0], sender[1])
                send_data = bytes(create_response_string(), 'ascii')
                log.info(send_data)
                send_sock.sendto(send_data, sender)
            elif data.strip(b"\0").startswith(b"MPM-ECHO"):
                log.info("Received echo request from {sender}".format(
                    sender=sender[0])
                )
                send_data = data
                send_sock.sendto(send_data, sender)
    except Exception as err:
        log.error("Error: %s", err)
        log.error("Error type: %s", type(err))
        sock.close()
        send_sock.close()
        exit(1)
