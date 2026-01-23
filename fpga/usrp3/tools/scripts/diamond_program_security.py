#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Inserts the MachXO3 sysCONFIG Programming Command "Program Security" into an
# existing SVF file.
#
import argparse
import os
import sys


def parse_arguments():
    parser = argparse.ArgumentParser(description="Insert Program Security command into SVF file")
    parser.add_argument("filename", type=str, help="The svf file to modify")
    parser.add_argument(
        "-o", "--output",
        type=str,
        default=None,
        help="Optional output file. If not specified, input file is overwritten."
    )
    return parser.parse_args()


if __name__ == '__main__':
    args = parse_arguments()

    # process the file line by line
    with open(args.filename, "r") as f:
        lines = f.readlines()

    # find the end of the programming sequence
    for i, line in enumerate(lines):
        if "! Program SECURITY" in line.strip():
            print("Program Security command already present in SVF file!")
            sys.exit(0)  # already present
        if "! Exit the programming mode" in line.strip():
            end_index = i
            break
    else:
        raise Exception("Error: Could not find end of programming sequence")

    # insert program security command before the end of programming sequence
    program_security_cmd = ["! Program SECURITY",
                            "",
                            "! Shift in ISC PROGRAM SECURITY(0xCE) instruction",
                            "SIR\t8\tTDI  (CE);",
                            "RUNTEST\tIDLE\t2 TCK\t1.00E-03 SEC;",
                            "",
                            ""]

    # add linebreak to each line
    program_security_cmd = [line + os.linesep for line in program_security_cmd]

    lines = lines[:end_index] + program_security_cmd + lines[end_index:]

    # write the modified lines back to the file
    output_file = args.output if args.output else args.filename
    with open(output_file, "w") as f:
        f.writelines(lines)
