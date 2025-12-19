#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

"""Utility to extract variables from CMake files or CMakeCache.txt for use in CI pipelines.

Supports both bash and batch (Windows) usage.
"""

import argparse
import os
import subprocess
import tempfile

#
# Script is intended for use from pipelines, called from either bash (linux) or batch (win) steps
#  Usage from bash:
#   eval $(python extract_cmake_var.py CMakeCache.txt VAR1 VAR2)
#
#     echo "$VAR1"
#     echo "$VAR2"
#
#  Usage from batch:
#   for /f "usebackq tokens=1,* delims==" %%A in ^
#     (`python extract_cmake_var.py CMakeCache.txt VAR1 VAR2`) do set %%A=%%~B
#
#     echo %VAR1%
#     echo %VAR2%
#


def extract_cmake_variable(cmake_file, variable_names):
    """Extract value(s) of CMake variables from a .cmake file using cmake -P.

    Args:
        cmake_file (str): Path to the .cmake file.
        variable_names (str or list): The variable(s) to extract.

    Returns:
        dict or str: Mapping of variable names to their values (missing variables get value None).
    """
    if isinstance(variable_names, str):
        variable_names = [variable_names]
    # Create a temporary CMake script
    with tempfile.NamedTemporaryFile("w", suffix=".cmake", delete=False) as tmp:
        tmp.write(f'include("{cmake_file}")\n')
        for var in variable_names:
            tmp.write(f'message(STATUS "{var}=${{{var}}}")\n')
        script_path = tmp.name
    results = {var: None for var in variable_names}
    try:
        try:
            result = subprocess.run(["cmake", "-P", script_path], capture_output=True, text=True)
        except FileNotFoundError as e:
            import sys

            print(
                "ERROR: 'cmake' was not found in your system PATH. Please ensure CMake is installed and its directory is added to your PATH environment variable.",
                file=sys.stderr,
            )
            print(f"System error: {e}", file=sys.stderr)
            os.remove(script_path)
            exit(1)
        for line in result.stdout.splitlines():
            for var in variable_names:
                if f"{var}=" in line:
                    # Example line: -- MY_VAR=somevalue
                    value = line.split(f"{var}=")[1].strip()
                    results[var] = value
    finally:
        os.remove(script_path)
    return results


def parse_cmake_cache(cache_path):
    """Extracts value(s) of all CMake variables from a CMakeCache file.

    Args:
        cache_path (str): Path to the CMakeCache file.

    Returns:
        dict: Mapping of variable names to their values.
    """
    with open(cache_path, encoding="utf-8") as f:
        return {
            var_type.split(":", 1)[0]: value
            for raw_line in f
            for line in [raw_line.strip()]
            if line
            and not line.startswith("//")
            and not line.startswith("#")
            and "=" in line
            and ":" in (var_type := line.split("=", 1)[0])
            and (value := line.split("=", 1)[1]) is not None
        }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=(
            "Utility for extracting variable values from cmake files\n\nAvailable source types:\n"
            "  cmake_source   Extract variables from a .cmake file using cmake -P.\n"
            "  cmake_cache    Extract variables from CMakeCache.txt.\n"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    subparsers = parser.add_subparsers(dest="type", required=True)

    # Action 1: Convert Tag-Value to JSON
    parser_cmake_source = subparsers.add_parser(
        "cmake_source", help="Extract variables from a .cmake file using cmake -P."
    )
    parser_cmake_source.add_argument("--cmake_file", required=True, help="Path to the .cmake file")
    parser_cmake_source.add_argument(
        "--variables", nargs="+", required=True, help="List of variables names to extract"
    )

    # Action 2:
    parser_cmake_cache = subparsers.add_parser(
        "cmake_cache", help="Extract variables from CMakeCache.txt"
    )
    parser_cmake_cache.add_argument("--cmake_file", required=True, help="Path to CMakeCache.txt")
    parser_cmake_cache.add_argument(
        "--variables", nargs="+", required=True, help="List of variables names to extract"
    )
    args = parser.parse_args()

    if args.type == "cmake_source":
        cache_vars = extract_cmake_variable(args.cmake_file, args.variables)
        for var in args.variables:
            value = cache_vars.get(var, "")
            print(f'{var}="{value}"')
    elif args.type == "cmake_cache":
        cache_vars = parse_cmake_cache(args.cmake_file)
        for var in args.variables:
            value = cache_vars.get(var, "")
            print(f'{var}="{value}"')
    else:
        parser.print_help()
