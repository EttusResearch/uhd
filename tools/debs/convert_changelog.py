#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2015 National Instruments Corp.
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
Converts our changelog into a format suitable for Debian packaging
"""

import datetime
from optparse import OptionParser
import os
import re
import sys

# Pass in first line of Debian changelog file, should contain last version
def detect_last_version(line):
    return convert_version_string(re.search("[0-9]+\.[0-9]+\.[0-9]", line).group(), False)

# "## 003.008.005" to "3.8.5" or vice versa
def convert_version_string(version, to_debian=True):
    if version == None:
        return ""

    if to_debian:
        return ".".join(list(str(int(num)) for num in re.split('[ .]', version)[1:]))
    else:
        return "## {0}".format(".".join("{0:03d}".format(int(num)) for num in version.split(".")))

#
# The "trusty" string below doesn't need to be changed, even when Trusty loses support. The script
# to upload packages replaces it anyway.
#
def get_header(version):
    return "uhd ({0}-0ubuntu1) trusty; urgency=low\n\n".format(convert_version_string(version))

def get_footer(uploader_name, uploader_email):
    return " -- {0} <{1}>  {2}\n\n".format(uploader_name, uploader_email, datetime.datetime.now().strftime("%a, %d %b %Y %I:%M:%S %Z-0800"))

if __name__ == "__main__":

    parser = OptionParser()
    parser.add_option(
        "--input-file",     type="string", default='CHANGELOG',
        help="Input UHD top-level changelog file"
    )
    parser.add_option("--output-file",
        type="string", default='host/cmake/debian/changelog',
        help="Output Debian changelog file (will append onto existing)"
    )
    parser.add_option("--uploader-name",
        type="string", default='Ettus Research',
        help="Uploader name (must match GPG key)",
    )
    parser.add_option("--uploader-email",
        type="string", default='packages@ettus.com',
        help="Uploader email (must match GPG key)"
    )
    parser.add_option("--last-version",   type="string", help="Manually specify last version (Debian format)", default="")
    (options, args) = parser.parse_args()

    # Input file
    f = open(options.input_file, "r")
    lines_in = f.readlines()
    f.close()

    lines_out = []

    g = open(options.output_file, "w")

    if options.last_version == "":
        if(len(lines_out) > 0):
            last_version = detect_last_version(lines_out[0])
            check_last_version = True
        else:
            last_version = ""
            check_last_version = False # Will do every version
    else:
        last_version = convert_version_string(options.last_version, False)
        check_last_version = True

    new_lines_out = []
    for line in lines_in[3:]:
        if line.rstrip() == last_version and last_version != "":
            # We've fully updated, stop here
            break
        elif re.search("^## [0-9]{3}\.[0-9]{3}\.[0-9]{3}", line):
            # New version
            new_lines_out += [get_header(line.rstrip())]
        elif line == "\n":
            # End of version
            new_lines_out += ["\n"]
            new_lines_out += [get_footer(options.uploader_name, options.uploader_email)]
        else:
            # Actual changes
            new_lines_out += ["  " + line]
    # Final footer
    new_lines_out += ["\n"]
    new_lines_out += [get_footer(options.uploader_name, options.uploader_email)]

    new_lines_out += lines_out
    for line in new_lines_out:
        g.write(line)
    g.close()
