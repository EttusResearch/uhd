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
import argparse
from gevent import signal
import usrp_mpm as mpm
from usrp_mpm.mpmtypes import SharedState
from usrp_mpm.periph_manager import periph_manager

_PROCESSES = []


def setup_arg_parser():
    """
    Create an arg parser
    """
    parser = argparse.ArgumentParser(description="USRP Hardware Daemon")
    parser.add_argument(
        '--daemon',
        help="Run as daemon",
        action="store_true",
    )
    parser.add_argument(
        '--init-only',
        help="Don't start the RPC server, terminate after running initialization",
        action="store_true",
    )
    parser.add_argument(
        '--override-db-pids',
        help="Provide a comma-separated list of daughterboard PIDs that are " \
             "used instead of whatever else the code may find",
        default=None
    )
    parser.add_argument(
        '--discovery-addr',
        help="Bind discovery socket to this address only. Defaults to all " \
             "addresses.",
        default="0.0.0.0",
    )
    parser.add_argument(
        '--default-args',
        help="Provide a comma-separated list of key=value pairs that are" \
             "used as defaults for device initialization.",
        default=None
    )
    return parser

def parse_args():
    """
    Return a fully parse args object
    """
    args = setup_arg_parser().parse_args()
    if args.override_db_pids is not None:
        args.override_db_pids = [int(x, 0) for x in args.override_db_pids.split(",")]
    args.default_args = args.default_args or ''
    try:
        args.default_args = {
            x.split('=')[0].strip(): x.split('=')[1].strip() if x.find('=') != -1 else ''
            for x in args.default_args.split(',')
            if len(x)
        }
    except IndexError:
        log.error("Could not parse default device args: `{}'".format(args.default_args))
    return args


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
    args = parse_args()
    shared = SharedState()
    # Create the periph_manager for this device
    # This call will be forwarded to the device specific implementation
    # e.g. in periph_manager/n310.py
    # Which implementation is called will be determined during configuration
    # with cmake (-DMPM_DEVICE).
    # mgr is thus derived from PeriphManagerBase (see periph_manager/base.py)
    log.info("Spawning periph manager...")
    mgr = periph_manager(args)
    discovery_info = {
        "type": mgr._get_device_info()["type"],
        "serial": mgr._get_device_info()["serial"]
    }
    if args.init_only:
        mgr.init(args.default_args)
        log.info("Terminating on user request before launching RPC server.")
        mgr.deinit()
        return True
    log.info("Spawning discovery process...")
    _PROCESSES.append(
        mpm.spawn_discovery_process(discovery_info, shared, args.discovery_addr)
    )
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

