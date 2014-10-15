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
Populates the current directory with a valid set of binaries for the
current commit.
"""

import os
import re
import subprocess
import uhdimgs

def get_md5_and_zipfilename():
    """ Return MD5 hash and ZIP filename from the host/CMakeLists.txt file. """
    cmakef = open(uhdimgs.get_cmake_main_file(), 'r').read()
    md5_regex = re.compile(r'UHD_IMAGES_MD5SUM\s*"(?P<md5>[0-9a-f]{32})', flags=re.MULTILINE)
    md5 = md5_regex.search(cmakef).groups('md5')[0]
    filename_regex = re.compile(r'UHD_IMAGES_DOWNLOAD_SRC\s*"(?P<filename>[^"]*\.zip)', flags=re.MULTILINE)
    filename = filename_regex.search(cmakef).groups('filename')[0]
    return (md5, filename)

def main():
    " Go, go, go! "
    # Switch to correct dir
    img_root_dir = os.path.join(uhdimgs.get_images_dir(), 'images')
    os.chdir(uhdimgs.get_images_dir())
    # Read out the CMakeLists.txt file, get filename
    print "== Reading MD5 and ZIP filename for current commit from {0}...".format(uhdimgs.get_cmake_main_file())
    (md5, filename) = get_md5_and_zipfilename()
    print "== Starting download..."
    try:
        downloader_cmd = [
                'python',
                '../host/utils/uhd_images_downloader.py.in',
                '-i', img_root_dir,
                '-f', filename,
                '-c', md5
        ]
        subprocess.check_call(downloader_cmd)
    except (subprocess.CalledProcessError, OSError):
        print "[ERROR] Failed to run downloader script."
        exit(1)
    print "== Done!"

if __name__ == "__main__":
    main()
