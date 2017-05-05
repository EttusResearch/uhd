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
import sys
from gevent import signal
import usrp_mpm as mpm
from usrp_mpm.mpmtypes import SharedState
from usrp_mpm.periph_manager import periph_manager

_PROCESSES = []

def kill_time(sig, frame):
    """
    kill all processes
    to be used in a signal handler

    If all processes are properly terminated, this will exit
    """
    log = mpm.get_main_logger().getChild('kill')
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
    log = mpm.get_main_logger().getChild('main')
    shared = SharedState()
    # Create the periph_manager for this device
    # This call will be forwarded to the device specific implementation
    # e.g. in periph_manager/n310.py
    # Which implementation is called will be determined during configuration
    # with cmake (-DMPM_DEVICE).
    # mgr is thus derived from PeriphManagerBase (see periph_manager/base.py)
    log.info("Spawning periph manager...")
    mgr = periph_manager()
    discovery_info = {
        "type": mgr._get_device_info()["type"],
        "serial": mgr._get_device_info()["serial"]
    }
    log.info("Spawning discovery process...")
    _PROCESSES.append(
        mpm.spawn_discovery_process(discovery_info, shared))
    log.info("Spawning RPC process...")
    _PROCESSES.append(
        mpm.spawn_rpc_process(mpm.mpmtypes.MPM_RPC_PORT, shared, mgr))
    log.info("Processes launched. Registering signal handlers.")
    signal.signal(signal.SIGTERM, kill_time)
    signal.signal(signal.SIGINT, kill_time)
    signal.pause()
    return True

if __name__ == '__main__':
    exit(not main())

