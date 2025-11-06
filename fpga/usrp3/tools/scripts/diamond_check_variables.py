#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Checks whether all variables in a Diamond lpf file are defined before usage.
# This check is not done in Diamond, so there is no error or warning message for us to
# check for.
#

import argparse
import re


def parse_arguments():
    parser = argparse.ArgumentParser(description="Check variables in a Diamond lpf file")
    parser.add_argument("filename", type=str, help="The lpf file to check")
    parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose output")
    return parser.parse_args()


args = parse_arguments()

defined_variables = set()
pattern = re.compile(r"(@define\s+)?\$(\w+)")

# search the file line by line
with open(args.filename, "r") as f:
    for line_number, line in enumerate(f, start=1):
        position = 0
        match = True
        while match:
            match = pattern.search(line, position)
            if match:
                # If group 1 is present we have a define and add the variable to the set
                if match.group(1):
                    defined_variables.add(match.group(2))
                    if args.verbose:
                        print(f"Line {line_number},{position}: Define variable {match.group(2)}")
                # Otherwise it is a usage and we check if the variable is defined
                else:
                    if args.verbose:
                        print(f"Line {line_number},{position}: Use of variable {match.group(2)}")
                    if match.group(2) not in defined_variables:
                        raise Exception(
                            f"Error: Variable {match.group(2)} is used before being defined"
                        )
                position = match.end(2)
