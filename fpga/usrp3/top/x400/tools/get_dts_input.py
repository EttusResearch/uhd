#!/usr/bin/env python3
#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
This script parses the target DTS file name and determines which DTS input
file to use as the source, then the DTS source file name to stdout. For
example, if the target is:

    build/usrp_x410_fpga_XG_100.dts

then it will print the input DTS file name (replacing the directory and
removing the _100):

    dts/usrp_x410_fpga_XG.dts

Note: This code assumes it's being run from the top x400 directory, because it
      will always add dts/ input file path.

usage: get_dts_input.py [-h] --target INPUT

Arguments:

  -h, --help            Show this help message and exit
  --target TARGET       The target file name (e.g., build/usrp_x410_fpga_X4_200.dts)

"""
import argparse
import os
import sys

def parse_args():
    """Parser for the command line arguments"""
    parser = argparse.ArgumentParser(description='Get input DTS path from target DTS path')
    parser.add_argument('--target', required=True, help='The name of the target DTS file')
    args = parser.parse_args()
    return args

def get_input(target):
    # Remove the path from the beginning
    dts_input = os.path.basename(target)
    # Remove the _XXX and extension from the end of the file name
    dts_input = "_".join(dts_input.split("_")[:-1])
    # Add path and extension
    dts_input = os.path.join('dts', dts_input + '.dts')
    return dts_input

def main():
    """ main function """
    args = parse_args()
    dts_input = get_input(args.target)
    print(dts_input)
    return True

if __name__ == '__main__':
    sys.exit(not main())
