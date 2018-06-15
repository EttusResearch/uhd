#!/usr/bin/env python
#
# Copyright 2014 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Populates the current directory with a valid set of binaries for the
current commit.
"""

from __future__ import print_function
import os
import subprocess
import uhdimgs

def download_images(img_root_dir=None):
    " Go, go, go! "
    # Switch to correct dir
    img_root_dir = img_root_dir or os.path.join(uhdimgs.get_images_dir(), 'images')
    if not os.path.isdir(img_root_dir):
        print("== Creating images directory...")
        os.mkdir(img_root_dir)
    os.chdir(uhdimgs.get_images_dir())
    print("== Starting download...")
    try:
        downloader_cmd = [
            'python',
            '../host/utils/uhd_images_downloader.py.in',
            '-i', img_root_dir,
            '-m', 'manifest.txt'
        ]
        subprocess.check_call(downloader_cmd)
    except (subprocess.CalledProcessError, OSError):
        print("[ERROR] Failed to run downloader script.")
        exit(1)
    print("== Done!")

if __name__ == "__main__":
    download_images()
