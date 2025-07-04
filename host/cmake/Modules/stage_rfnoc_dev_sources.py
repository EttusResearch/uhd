#!/usr/bin/env python3
"""Copy the usrp3 directory into the build directory for installation.

Note: This only copies files that are relevant for RFNoC development.
"""

import argparse
import fnmatch
import os
import shutil
import sys

# List of patterns to ignore when copying files. Are always applied, regardless
# of the current directory.
IGNORE_PATTERNS = [
    "*.pyc",
    "*.pyo",
    "__pycache__",
    ".git",
    "build*",
    "diamond_*",
    "quartus_*",
    "dmd_design_build.tcl",
    "ise_jtag_program.sh",
]

SKIP_PATHS = [
    "top/b200",
    "top/b2xxmini",
    "top/build.py",
    "tools/utils",
    "lib/control_200",
    "lib/fifo_200",
    "lib/packet_proc_200",
    "lib/radio_200",
    "lib/vita_200",
]


def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description="Copy usrp3 directory for installation.")
    parser.add_argument(
        "--src",
        type=str,
        required=True,
        help="Path to the usrp3 directory that will be staged.",
    )
    parser.add_argument(
        "--dest",
        type=str,
        required=True,
        help="Path to the build directory where usrp3 will be staged.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable verbose output.",
    )
    return parser.parse_args()


def main():
    """Make it so."""
    args = parse_args()

    src_dir = os.path.normpath(os.path.abspath(args.src))
    dst_dir = os.path.normpath(os.path.abspath(args.dest))
    verbose = args.verbose

    def ignore(path, names):
        """Ignore certain files and directories.

        Reminder: Return a subset of `names` that should be ignored.
        """
        ignore_list = [
            name
            for name in names
            if any(fnmatch.fnmatch(name, pattern) for pattern in IGNORE_PATTERNS)
            or name.startswith(".")
            or os.path.relpath(os.path.join(path, name), src_dir) in SKIP_PATHS
        ]
        if verbose:
            for ig in ignore_list:
                print("Ignoring: {}".format(os.path.join(path, ig)))
        return ignore_list

    print("Copying files from {} to {}...".format(src_dir, dst_dir))
    if not os.path.exists(args.src):
        print("Source directory does not exist: {}".format(args.src))
        return False

    def copy_verbose(src, dst, *args):
        """Copy files with verbose output."""
        print(f"Copying {src} to {dst}")
        shutil.copy2(src, dst, *args)

    shutil.copytree(
        args.src,
        args.dest,
        dirs_exist_ok=False,
        ignore=ignore,
        copy_function=copy_verbose if verbose else shutil.copy2,
    )
    return True


if __name__ == "__main__":
    sys.exit(not main())
