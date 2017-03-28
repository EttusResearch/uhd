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
from logging import getLogger
from logging import StreamHandler
from logging import DEBUG
from logging import Formatter
import signal
import sys
import usrp_mpm as mpm
from usrp_mpm.types import shared_state
from usrp_mpm.periph_manager import periph_manager

log = getLogger("usrp_mpm")
_PROCESSES = []


def kill_time(signal, frame):
    """
    kill all processes
    to be used in a signal handler
    """
    for proc in _PROCESSES:
        proc.terminate()
        log.info("Terminating pid: {0}".format(proc.pid))
    for proc in _PROCESSES:
        proc.join()
    log.info("System exiting")
    sys.exit(0)


def main():
    """
    Go, go, go!

    Main process loop.
    """
    # Setup logging
    log.setLevel(DEBUG)
    ch = StreamHandler()
    ch.setLevel(DEBUG)
    formatter = Formatter('[%(asctime)s] [%(levelname)s] [%(name)s] [%(message)s]')
    ch.setFormatter(formatter)
    log.addHandler(ch)


    shared = shared_state()
    mgr = periph_manager()
    _PROCESSES.append(
        mpm.spawn_discovery_process({'serial': mgr.get_serial()}, shared))
    _PROCESSES.append(
        mpm.spawn_rpc_process(mpm.types.MPM_RPC_PORT, shared, mgr))
    signal.signal(signal.SIGTERM, kill_time)
    signal.signal(signal.SIGINT, kill_time)
    signal.pause()

if __name__ == '__main__':
    exit(not main())
