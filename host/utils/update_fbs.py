#!/usr/bin/env python3
#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utility to update the .fbs files in UHD, or to verify them.
"""

import os
import sys
import glob
import argparse
import filecmp
import pathlib
import tempfile
import shutil
import subprocess

CAL_SUBDIR = ('include', 'uhd', 'cal')

def find_flatc(hint):
    """
    Return a valid path to flatc or throw otherwise.
    """
    def is_executable(filename):
        """ Check filename points to an executable """
        return os.path.isfile(filename) and os.access(filename, os.X_OK)
    if hint:
        if is_executable(hint):
            return hint
        raise RuntimeError("Not a valid path to a flatc executable: `{}'"
                           .format(hint))
    for path in os.environ.get("PATH", "").split(os.pathsep):
        flatc_exe = os.path.join(path, 'flatc')
        if is_executable(flatc_exe):
            return flatc_exe
    raise RuntimeError("Could not find flatc in $PATH.")

def find_uhd_source_path(hint):
    """
    Find UHD path
    """
    if hint:
        cal_subdir = os.path.join(hint, *CAL_SUBDIR)
        if os.path.isdir(cal_subdir):
            return hint
        hint = os.path.join(hint, 'host')
        if os.path.isdir(cal_subdir):
            return hint
        raise RuntimeError(
            "Invalid UHD source path: {} (does not have subdir: {})"
            .format(hint, os.path.join(*CAL_SUBDIR)))
    # If there's no hint, we try our own path as a hint:
    return find_uhd_source_path(str(pathlib.Path().absolute().parent))

def parse_args():
    """ Parse args and return args object """
    parser = argparse.ArgumentParser(
        description="Update or verify FlatBuffer files in UHD",
    )
    parser.add_argument(
        '--flatc-exe',
        help="Path to flatc executable. Will attempt to find the executable if "
             "not provided.")
    parser.add_argument(
        '--uhd-path',
        help="Path to UHD repository. Will use the repository this file is in "
             "if not provided.")
    parser.add_argument(
        "-V", "--verify", action='store_true',
        help="If set, will only check if the files are up-to-date, and return "
             "non-zero if they are not.")
    return parser.parse_args()

def verify_fbs(flatc_exe, files):
    """
    Make sure that the .fbs files are all up to date w.r.t. their generated
    files. This is accomplished by re-generating them in an temp dir and diffing
    the new files with the existing ones.
    """
    tmp_dir = tempfile.mkdtemp()
    # Generate them all again, but put them in a temp dir
    subprocess.check_call([flatc_exe, '-o', tmp_dir, '--cpp'] + files)
    uhd_gen_files = sorted(glob.glob("*generated.h"))
    tmp_gen_files = sorted(glob.glob(os.path.join(tmp_dir, '*generated.h')))
    if len(uhd_gen_files) != len(tmp_gen_files):
        print("List of generated files does not match list of .fbs files!")
        return False
    try:
        result = True
        for uhd_file, tmp_file in zip(uhd_gen_files, tmp_gen_files):
            print("Checking {}... ".format(uhd_file), end="")
            if not filecmp.cmp(uhd_file, tmp_file):
                print("ERROR")
                result = False
            else:
                print("OK")
        return result
    except BaseException as ex:
        print("ERROR: " + str(ex))
        return False
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)

def main():
    """ Go, go, go! """
    args = parse_args()
    flatc_exe = find_flatc(args.flatc_exe)
    print("Found flatc executable: {}".format(flatc_exe))
    try:
        cal_path = os.path.join(find_uhd_source_path(args.uhd_path), *CAL_SUBDIR)
    except RuntimeError as ex:
        print("ERROR: {}".format(str(ex)))
        return False
    print("Checking UHD cal data in: {}".format(cal_path))
    os.chdir(cal_path)
    files = glob.glob("*.fbs")
    if args.verify:
        return not verify_fbs(flatc_exe, files)
    subprocess.check_call([flatc_exe, '--cpp'] + files)
    return True

if __name__ == "__main__":
    sys.exit(not main())
