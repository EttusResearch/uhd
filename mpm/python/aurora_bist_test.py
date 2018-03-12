#!/usr/bin/env python3
#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Aurora BIST command line utility
"""

from __future__ import print_function
import argparse
import usrp_mpm as mpm
from usrp_mpm.sys_utils.uio import UIO
from usrp_mpm.aurora_control import AuroraControl

########################################################################
# command line options
########################################################################
def parse_args():
    " Create argparser, return args "
    parser = argparse.ArgumentParser(
        description='Controller for Ettus Aurora BIST Engine'
    )
    parser.add_argument(
        '--uio-dev', default='misc-auro-regs0',
        help='UIO device for master device peeks and pokes'
    )
    parser.add_argument(
        '--base-addr', type=int, default=0,
        help='Base address for register read/writes'
    )
    parser.add_argument(
        '--slave-uio-dev', default=None,
        help='UIO device for slave device peeks and pokes'
    )
    parser.add_argument(
        '--slave-base-addr', type=int, default=0,
        help='Base address for register read/writes on slave'
    )
    parser.add_argument(
        '--test', default='ber', choices=['ber', 'latency'],
        help='Type of test to run'
    )
    parser.add_argument(
        '--duration', type=int, default=10, help='Duration of test in seconds'
    )
    parser.add_argument(
        '--rate', type=int, default=1245, help='BIST throughput in MB/s'
    )
    parser.add_argument(
        '--loopback', action="store_true",
        help="Don't run a test, but set this Aurora into loopback mode"
    )
    return parser.parse_args()

########################################################################
# main
########################################################################
def main():
    " Dawaj, dawaj! "
    args = parse_args()
    # Initialize logger for downstream components
    mpm.get_main_logger().getChild('main')
    master_core_uio = UIO(label=args.uio_dev, read_only=False)
    master_core_uio.open()
    if args.slave_uio_dev:
        slave_core_uio = UIO(label=args.uio_dev, read_only=False)
        slave_core_uio.open()
    try:
        master_core = AuroraControl(master_core_uio, args.base_addr)
        slave_core = None if args.slave_uio_dev is None else AuroraControl(
            slave_core_uio,
            args.slave_base_addr,
        )
        if args.loopback:
            master_core.reset_core()
            master_core.set_loopback(enable=True)
            return True
        # Run BIST
        if args.test == 'ber':
            print("Performing BER BIST test.")
            master_core.run_ber_loopback_bist(
                args.duration,
                args.rate * 8e6,
                slave_core,
            )
        else:
            print("Performing Latency BIST test.")
            master_core.run_latency_loopback_bist(
                args.duration,
                args.rate * 8e6,
                slave_core,
            )
    except Exception as ex:
        print("Unexpected exception: {}".format(str(ex)))
        return False
    finally:
        master_core_uio.close()
        if args.slave_uio_dev:
            slave_core_uio.close()

if __name__ == '__main__':
    exit(not main())
