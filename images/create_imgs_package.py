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

import re
import os
import uhdimgs
import glob
import subprocess
import argparse
import shutil

def clear_img_dir(img_root_dir):
    """ Removes non-image files from the images dir """
    globs = ["*.tag", "LICENSE"]
    for the_glob in globs:
        for filename in glob.iglob(os.path.join(img_root_dir, the_glob)):
            print 'Removing file from images directory: ', filename
            os.unlink(filename)

def get_zipfilename_from_cpack_output(cpoutput):
    """ Parses the output of the ZIP-file creating script
    and scrapes the actual file name. """
    regex = re.compile("\/build\/(?P<filename>[^\/]+\.zip)")
    results = regex.search(cpoutput)
    return results.group('filename')

def parse_args():
    """ Parse args, duh """
    parser = argparse.ArgumentParser(description='Link the current set of images to this commit.')
    parser.add_argument('--commit', default=None,
                       help='Supply a commit message to the changes to host/CMakeLists.txt.')
    parser.add_argument('-r', '--release-mode', default=None,
                       help='Specify UHD_RELEASE_MODE. Typically "release" or "rc1" or similar.')
    parser.add_argument('--skip-edit', default=False, action='store_true',
                       help='Do not edit the CMakeLists.txt file.')
    parser.add_argument('--skip-move', default=False, action='store_true',
                       help='Do not move the archives after creating them.')
    return parser.parse_args()

def move_zip_to_repo(base_url, zipfilename):
    final_destination = os.path.join(base_url, zipfilename)
    if os.path.exists(final_destination):
        print "WARNING: A file with name {0} is already in the images repository.".format(zipfilename)
        print "Overwrite? [y/N]",
        ans = raw_input()
        if ans.strip().upper() != 'Y':
            return
        os.unlink(final_destination)
    shutil.move(zipfilename, base_url)

def main():
    " Go, go, go! "
    args = parse_args()
    img_root_dir = os.path.join(uhdimgs.get_images_dir(), 'images')
    os.chdir(uhdimgs.get_images_dir())
    print "== Clearing out the images directory..."
    clear_img_dir(img_root_dir)
    print "== Creating archives..."
    cpack_cmd = ["./make_zip.sh",]
    if args.release_mode is not None:
        cpack_cmd.append(args.release_mode)
    try:
        cpack_output = subprocess.check_output(cpack_cmd)
    except subprocess.CalledProcessError as e:
        print e.output
        raise SystemExit, 1
    zipfilename = get_zipfilename_from_cpack_output(cpack_output)
    print "Filename: ", zipfilename
    print "== Calculating MD5 sum of ZIP archive..."
    md5 = uhdimgs.md5_checksum(zipfilename)
    print 'MD5: ', md5
    base_url = uhdimgs.get_base_url()
    if not args.skip_move and uhdimgs.base_url_is_local(base_url) and os.access(base_url, os.W_OK):
        print "== Moving ZIP file to {0}...".format(base_url)
        move_zip_to_repo(base_url, zipfilename)
    print "== Updating CMakeLists.txt..."
    uhdimgs.update_main_cmake_file(md5, zipfilename)
    if args.commit is not None:
        print "== Committing changes..."
        subprocess.check_call(['git', 'commit', '-m', args.commit, uhdimgs.get_cmake_main_file()])
    print "== Done!"

if __name__ == "__main__":
    main()
