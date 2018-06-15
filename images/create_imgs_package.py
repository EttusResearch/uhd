#!/usr/bin/env python
#
# Copyright 2014 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
"""
Command-line utility to create a .zip-file with the current image set.
"""

from __future__ import print_function
import re
import os
import subprocess
import argparse
import shutil
import uhdimgs

def parse_args():
    """ Parse args, duh """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--version',
        help="Specify version. Will detect from tag otherwise."
    )
    return parser.parse_args()

def download_images(img_root_dir):
    """
    Run the images downloader
    """
    import populate_images
    populate_images.download_images(img_root_dir)

def get_version():
    """
    Figure out version based on tag.
    """
    try:
        git_cmd = ['git', 'describe', '--abbrev=0', '--tags']
        git_output = subprocess.check_output(git_cmd)
    except subprocess.CalledProcessError as ex:
        print(ex.output)
        exit(1)
    print("Detected tag: {}".format(git_output))
    version_mobj = re.search(r'[0-9]+\.[0-9]+\.[0-9]+\.[0-9]', git_output)
    if version_mobj is None:
        print("Error: Failure to resolve version from tag!")
        exit(1)
    return version_mobj.group(0)

def main():
    """ Go, go, go! """
    args = parse_args()
    img_root_dir = os.path.join(uhdimgs.get_images_dir(), 'images')
    print("== Clearing out the images directory...")
    shutil.rmtree(img_root_dir)
    print("== Downloading images...")
    download_images(img_root_dir)
    print("== Determining version...")
    version = args.version if args.version is not None else get_version()
    print("Version string: {}".format(version))
    print("== Creating archives...")
    archive_cmd = ["./make_zip.sh", version]
    try:
        subprocess.call(archive_cmd)
    except subprocess.CalledProcessError as ex:
        print(ex.output)
        exit(1)
    print("== Done!")

if __name__ == "__main__":
    main()
