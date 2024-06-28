#!/usr/bin/env python3
#
# Copyright 2014 Ettus Research LLC
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Command-line utility to create a .zip-file with the current image set."""

import argparse
import os
import re
import shutil
import subprocess
import sys

import populate_images


def parse_args():
    """Parse args, duh."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", help="Specify version. Will detect from tag otherwise.")
    return parser.parse_args()


def download_images(img_root_dir):
    """Run the images downloader."""
    populate_images.download_images(img_root_dir)


def get_version():
    """Figure out version based on tag."""
    try:
        git_cmd = ["git", "describe", "--abbrev=0", "--tags"]
        git_output = subprocess.check_output(git_cmd).decode("UTF-8")
    except subprocess.CalledProcessError as ex:
        print((ex.output))
        sys.exit(1)
    print(("Detected tag: {}".format(git_output)))
    version_mobj = re.search(r"[0-9]+\.[0-9]+\.[0-9]+\.[0-9]", git_output)
    if version_mobj is None:
        print("Error: Failure to resolve version from tag!")
        sys.exit(1)
    return version_mobj.group(0)


def main():
    """Go, go, go."""
    args = parse_args()
    img_root_dir = os.path.join(populate_images.get_images_dir(), "images")
    print(f"== Clearing out the images directory at {img_root_dir}, if present...")
    shutil.rmtree(img_root_dir, ignore_errors=True)
    print("== Downloading images...")
    download_images(img_root_dir)
    print("== Determining version...")
    version = args.version if args.version is not None else get_version()
    print(f"Version string: {version}")
    print("== Creating archives...")
    archive_cmd = ["./make_zip.sh", version]
    try:
        subprocess.call(archive_cmd)
    except subprocess.CalledProcessError as ex:
        print(ex.output)
        sys.exit(1)
    print("== Done!")


if __name__ == "__main__":
    main()
