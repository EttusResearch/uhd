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
Main executable for the USRP Hardware Daemon
"""

from __future__ import print_function
from multiprocessing import Value
import ctypes
import signal
import time
import usrp_mpm as mpm
from usrp_mpm.types import shared_state
from usrp_mpm.types import graceful_exit


def signal_handler(signum, frame):
    raise graceful_exit()


def main():
    """
    Go, go, go!

    Main process loop.
    """
    procs = []
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    shared = shared_state()
    procs.append(mpm.spawn_discovery_process({'a': "foo"}, shared))
    # procs.append(mpm.spawn_rpc_process("Echo", 5000))
    procs.append(mpm.spawn_rpc_process("MPM", mpm.types.MPM_RPC_PORT, shared))
    try:
        for proc in procs:
            proc.join()
    except mpm.types.graceful_exit:
        pass

if __name__ == '__main__':
    exit(not main())

