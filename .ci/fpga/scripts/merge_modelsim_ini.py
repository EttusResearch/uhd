#!/usr/bin/env python3
"""
Copyright 2025 Ettus Research, a National Instruments Brand

SPDX-License-Identifier: LGPL-3.0-or-later

Merge multiple INI files into a single one.
"""

import configparser
import argparse

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('files', type=str, nargs='+', help='a list of INI files to merge')
parser.add_argument('-o', '--output', type=str, default='modelsim.ini', help='the output file name')
args = parser.parse_args()

config = configparser.ConfigParser()
config.read(args.files)

with open(args.output, 'w') as configfile:
    config.write(configfile)
