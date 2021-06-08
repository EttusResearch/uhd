#!/usr/bin/env python3
#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#
#   Our pipelines reuse IP builds to save time. In the case where an IP build
#   fails or is terminated before completion, it is sometimes necessary to
#   clean up the build so that the next build can complete properly. This
#   script searches for and deletes any incomplete IP builds.
#

import argparse
import os
import shutil

# argument parsing
parser = argparse.ArgumentParser(description="This script removes all directories where the .build_lock file still exists")
parser.add_argument('--directory', '-d', required=True, help='directory to search for lock files (recursively)')
args = parser.parse_args()

# search .build_lock files
lockFiles = []
for root, dirs, files in os.walk(args.directory):
  for file in files:
    if file == ".build_lock":
      lockFiles.append(os.path.join(root, file))

# remove all directories containing lock files
for lockFile in lockFiles:
  dirPath = os.path.dirname(lockFile)
  print("delete " + dirPath)
  shutil.rmtree(dirPath)
