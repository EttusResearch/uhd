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
import glob
import argparse
import pathlib
import re
import shutil
import subprocess
import sys

CAL_SUBDIR = ('include', 'uhd', 'cal')

def find_executable(name, hint=None):
    """
    Find an executable file. See documentation of which
    for platform depended behaviour.
    """
    result = shutil.which(name, path=hint)
    print("Found {} executable: {}".format(name, result))
    return result

def find_uhd_source_path(hint=None):
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
    return find_uhd_source_path(str(pathlib.Path(__file__).parent.absolute().parent))

def parse_args():
    """ Parse args and return args object """
    parser = argparse.ArgumentParser(
        description="Update or verify FlatBuffer files in UHD",
    )
    parser.add_argument(
        '--flatc-path',
        help="Path to flatc executable. Will attempt to find the executable if "
             "not provided.")
    parser.add_argument(
        '--git-path',
        help="Path to git executable. Will attempt to find the executable if "
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

def get_schema_files(uhd_path=None):
    """
    Returns a list of flatbuffers schema files (using glob) in uhd_path
    """
    try:
        cal_path = os.path.join(find_uhd_source_path(uhd_path), *CAL_SUBDIR)
        cal_path = os.path.abspath(cal_path)
    except RuntimeError as ex:
        print("ERROR: {}".format(str(ex)))
        return False

    print("Checking UHD cal data in: {}".format(cal_path))
    os.chdir(cal_path)
    return glob.glob("*.fbs")

def get_hash(git_exe, file):
    """
    return the latest git hash of file. Returns None if git does not return 0.
    """
    try:
        # git command to extract the hash of the last commit
        git_cmd = (git_exe, "log", "-1", "--oneline", "--pretty='%h'")
        result = subprocess.check_output(git_cmd + (file,), stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as error:
        print("Failed to read hash from {} ({})".format(file, error.output))
        result = None
    return result


def verify(git_exe, uhd_path=None):
    """
    Make sure that the .fbs files are all up to date w.r.t. their generated
    files. Because the content of the generated headers differ between the
    versions of flatbuffers we cannot compare generated files with files
    in repo.
    Instead the git hashes of the schema and the generated headers files
    are compared. This will detect changes to the .fbs that are not
    accompanied by a change of the header. It also detects manual
    changes to the generated header files.
    """
    if not git_exe:
        print("Cannot verify schema files (no git found), assuming pass")
        return True
    try:
        subprocess.check_output((git_exe, "status"), stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError:
        print("Cannot verify schema files (not a git repo), assuming pass")
        return True
    try:
        result = True
        for file in get_schema_files(uhd_path):
            print(file, end="...")
            fbs_hash = get_hash(git_exe, file)
            hpp_hash = get_hash(git_exe,
                                re.sub(r'\.fbs$', '_generated.h', file))
            if fbs_hash and hpp_hash:
                # we have a valid hash for both files
                if fbs_hash == hpp_hash:
                    print("OK")
                else:
                    print("ERROR git hashes of schema {} and header {} differ."
                          .format(fbs_hash, hpp_hash))
                    result = False
        return result
    except BaseException as ex:
        print("ERROR: " + str(ex))
        return False

def generate(flatc_exe, uhd_path=None):
    """
    Generate header files from schema
    """
    files = get_schema_files(uhd_path)
    subprocess.check_call([flatc_exe, '--cpp'] + files)
    return True

def main():
    """ Go, go, go! """
    args = parse_args()
    if args.verify:
        git_exe = find_executable("git", hint=args.git_path)
        return verify(git_exe, uhd_path=args.uhd_path)

    flatc_exe = find_executable("flatc", hint=args.flatc_path)
    return generate(flatc_exe, uhd_path=args.uhd_path)

if __name__ == "__main__":
    sys.exit(not main())
