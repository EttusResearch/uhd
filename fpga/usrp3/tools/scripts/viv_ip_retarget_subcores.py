#
# Copyright 2022 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

""" Vivado IP sub-core re-targeting.

    Script to update the sub-cores in a directory to a specific
    part number
"""

import argparse
import os
import re
import sys


# Parse command line options
def get_options():
    """Parse script arguments."""
    parser = argparse.ArgumentParser(
        description="Utility script to modify subcores for a Xilinx IP XCI file"
    )
    parser.add_argument(
        "xci_filepath", type=str, default=None, help="Name for the IP core"
    )
    parser.add_argument(
        "--target",
        type=str,
        default=None,
        help="Input value for target. Must be of the form <arch>/<device>/<package>/<speedgrade>/<silicon revision>",
    )
    parser.add_argument(
        "--name", type=str, default=None, help="Input value for new IP name"
    )
    parser.add_argument(
        "--output_dir", type=str, default=".", help="Build directory for IP"
    )
    args = parser.parse_args()
    if not args.xci_filepath:
        print("ERROR: Please specify the location for the XCI file to operate on\n")
        parser.print_help()
        sys.exit(1)
    if not os.path.isfile(args.xci_filepath):
        print(
            "ERROR: XCI File "
            + args.xci_filepath
            + " could not be accessed or is not a file.\n"
        )
        parser.print_help()
        sys.exit(1)
    return args


def get_match_str(item):
    """Produce the string to an XML parameter with a value."""
    return (
        r'(.*\<spirit:configurableElementValue spirit:referenceId=".*\.'
        + item
        + r'"\>)(.+)(\</spirit:configurableElementValue\>)'
    )


def get_empty_match_str(item):
    """Produce the string to an XML parameter without a value."""
    return (
        r'(.*\<spirit:configurableElementValue spirit:referenceId=".*\.'
        + item
        + r'")/\>'
    )


def retarget_subcore(path, file, output_dir, target_tok):
    """Update target parameter for the specified sub-core."""
    # Read XCI File
    with open(os.path.join(path, file)) as in_file:
        xci_lines = in_file.readlines()

    replace_dict = {
        "ARCHITECTURE": target_tok[0],
        "DEVICE": target_tok[1],
        "PACKAGE": target_tok[2],
        "SPEEDGRADE": target_tok[3],
        "C_XDEVICEFAMILY": target_tok[0],
        "C_FAMILY": target_tok[0],
        "C_XDEVICE": target_tok[1],
    }
    if len(target_tok) > 4:
        replace_dict["TEMPERATURE_GRADE"] = target_tok[4]
    if len(target_tok) > 5:
        replace_dict["SILICON_REVISION"] = target_tok[5]
    out_xci_filename = os.path.join(os.path.abspath(output_dir), os.path.basename(file))

    # Write files with modified parameters
    with open(out_xci_filename, "w") as out_file:
        for r_line in xci_lines:
            w_line = r_line
            m = re.search(
                get_match_str("(" + "|".join(list(replace_dict.keys())) + ")"), r_line
            )
            if m is not None:
                w_line = m.group(1) + replace_dict[m.group(2)] + m.group(4) + "\n"
            else:
                m = re.search(
                    get_empty_match_str(
                        "(" + "|".join(list(replace_dict.keys())) + ")"
                    ),
                    r_line,
                )
                if m is not None:
                    w_line = (
                        m.group(1)
                        + ">"
                        + replace_dict[m.group(2)]
                        + "</spirit:configurableElementValue>\n"
                    )
            out_file.write(w_line)


def main():
    """Perform directory traversal to detect and update sub-cores."""
    args = get_options()

    # Validate arguments
    if not os.path.isdir(args.output_dir):
        print(
            "ERROR: IP Build directory "
            + args.output_dir
            + " could not be accessed or is not a directory."
        )
        sys.exit(1)
    if not args.target:
        print("ERROR: No target specified.")
        sys.exit(1)
    target_tok = args.target.split("/")
    if len(target_tok) < 4:
        print(
            "ERROR: Invalid target format. Must be <arch>/<device>/<package>/<speedgrade>/<tempgrade>/<silicon revision>"
        )
        sys.exit(1)

    sub_cores = []

    # Detect sub-cores
    ip_dir, ip_name = os.path.split(args.xci_filepath)
    for path, _, files in os.walk(ip_dir):
        for name in files:
            if name.endswith("xci"):
                sub_cores.append(os.path.join(path, name))

    # Get the relative path for each IP core detected
    relative_paths = [os.path.relpath(path, ip_dir) for path in sub_cores]

    # Remove main core
    relative_paths.remove(ip_name)

    for rp in relative_paths:
        retarget_subcore(
            os.path.join(ip_dir, os.path.split(rp)[0]),
            os.path.split(rp)[1],
            os.path.join(args.output_dir, os.path.split(rp)[0]),
            target_tok,
        )


if __name__ == "__main__":
    main()
