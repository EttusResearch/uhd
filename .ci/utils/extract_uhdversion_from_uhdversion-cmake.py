#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Python utility to extract UHD version from UHDVersion.cmake."""

import argparse
import re
import sys


def extract_uhd_version(cmake_path, version_format):
    """Extract UHD_VERSION_* from UHDVersion.cmake and print in requested format."""
    pattern_int = re.compile(r"set\s*\(\s*UHD_VERSION_([A-Z]+)\s+([0-9]+)\s*\)")
    pattern_bool = re.compile(r"set\s*\(\s*UHD_VERSION_([A-Z]+)\s+(TRUE|FALSE)\s*\)")
    with open(cmake_path, encoding="utf-8") as f:
        values = {key.lower(): version for key, version in pattern_int.findall(f.read())}
        f.seek(0)  # Reset file pointer to beginning
        values.update({key.lower(): version for key, version in pattern_bool.findall(f.read())})
    if all(k in values for k in ("major", "api", "abi", "patch", "devel")):
        major = values["major"]
        api = values["api"]
        abi = values["abi"]
        patch = values["patch"]
        devel = values["devel"] == "TRUE"

        if version_format == "api":
            print(f"{major}.{api}")
        elif version_format == "abi":
            print(f"{major}.{api}.{abi}")
        elif version_format == "full":
            # Using a large number to ensure devel versions are allways
            # treated as newer than api compatible release versions.
            # Note: Assumes consumer supports at least 16-bit version fields.
            forth_digit = "9999" if devel else patch
            print(f"{major}.{api}.{abi}.{forth_digit}")
        elif version_format == "raw":
            print(f"{major}.{api}.{abi}.{patch}")
    else:
        print("Could not extract all version components.", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Extract UHD version from UHDVersion.cmake with configurable digit count."
    )
    parser.add_argument("cmake_file", help="Path to UHDVersion.cmake")
    parser.add_argument(
        "--format",
        choices=["api", "abi", "full", "raw"],
        default="full",
        help="Version format: api, abi, full, raw",
    )
    args = parser.parse_args()

    extract_uhd_version(args.cmake_file, args.format)
