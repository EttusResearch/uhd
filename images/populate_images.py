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
import subprocess
import uhdimgs

def main():
    " Go, go, go! "
    # Switch to correct dir
    img_root_dir = os.path.join(uhdimgs.get_images_dir(), 'images')
    os.chdir(uhdimgs.get_images_dir())
    print "== Starting download..."
    try:
        downloader_cmd = [
                'python',
                '../host/utils/uhd_images_downloader.py.in',
                '-i', img_root_dir,
                '-m', 'manifest.txt'
        ]
        subprocess.check_call(downloader_cmd)
    except (subprocess.CalledProcessError, OSError):
        print "[ERROR] Failed to run downloader script."
        exit(1)
    print "== Done!"

if __name__ == "__main__":
    main()
