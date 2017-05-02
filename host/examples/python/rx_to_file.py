#!/usr/bin/env python
#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
RX samples to file using Python API
"""

import argparse
import numpy as np
import uhd


def parse_args():
    """Parse the command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--args", default="", type=str)
    parser.add_argument("-o", "--output-file", type=str, required=True)
    parser.add_argument("-f", "--freq", type=float, required=True)
    parser.add_argument("-r", "--rate", default=1e6, type=float)
    parser.add_argument("-d", "--duration", default=5.0, type=float)
    parser.add_argument("-c", "--channels", default=0, nargs="+", type=int)
    parser.add_argument("-g", "--gain", type=int, default=10)
    parser.add_argument("-n", "--numpy", default=False, action="store_true",
                        help="Save output file in NumPy format (default: No)")
    return parser.parse_args()


def main():
    """RX samples and write to file"""
    args = parse_args()
    usrp = uhd.usrp.MultiUSRP(args.args)
    num_samps = int(np.ceil(args.duration*args.rate))
    if not isinstance(args.channels, list):
        args.channels = [args.channels]
    samps = usrp.recv_num_samps(num_samps, args.freq, args.rate, args.channels, args.gain)
    with open(args.output_file, 'wb') as out_file:
        if args.numpy:
            np.save(out_file, samps, allow_pickle=False, fix_imports=False)
        else:
            samps.tofile(out_file)

if __name__ == "__main__":
    main()
