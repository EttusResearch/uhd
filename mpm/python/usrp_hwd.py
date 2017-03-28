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
from systemd.journal import JournalHandler
from logging import DEBUG
from logging import Formatter
from gevent import signal
import sys
import usrp_mpm as mpm
from usrp_mpm.types import SharedState
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
    handler = StreamHandler()
    journal_handler = JournalHandler(SYSLOG_IDENTIFIER='usrp_hwd')
    handler.setLevel(DEBUG)
    formatter = Formatter('[%(asctime)s] [%(levelname)s] [%(module)s] %(message)s')
    handler.setFormatter(formatter)
    journal_formatter = Formatter('[%(levelname)s] [%(module)s] %(message)s')
    journal_handler.setFormatter(journal_formatter)
    log.addHandler(handler)
    log.addHandler(journal_handler)

    shared = SharedState()
    # Create the periph_manager for this device
    # This call will be forwarded to the device specific implementation
    # e.g. in periph_manager/test.py
    # Which implementation is called will be determined during configuration
    # with cmake (-DMPM_DEVICE)
    mgr = periph_manager()
    discovery_info = {
        "type": mgr._get_device_info()["type"],
        "serial": mgr._get_device_info()["serial"]
    }
    _PROCESSES.append(
        mpm.spawn_discovery_process(discovery_info, shared))
    _PROCESSES.append(
        mpm.spawn_rpc_process(mpm.types.MPM_RPC_PORT, shared, mgr))
    signal.signal(signal.SIGTERM, kill_time)
    signal.signal(signal.SIGINT, kill_time)
    signal.pause()

if __name__ == '__main__':
    exit(not main())
