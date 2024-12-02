#!/usr/bin/env python3
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
"""Convert our changelog into a format suitable for Debian packaging."""

import argparse
import datetime
import re


# Pass in first line of Debian changelog file, should contain last version
def detect_last_version(line):
    """Return the version string (e.g. 004.006.000.001) from line."""
    return convert_version_string(re.search(r"[0-9]+\.[0-9]+\.[0-9\.]+", line).group(), False)


def convert_version_string(version, to_debian=True):
    """Convert 003.008.005" to "3.8.5" or vice versa."""
    if version is None:
        return ""

    if to_debian:
        return ".".join(list(str(int(num)) for num in re.split("[ .]", version)[1:]))
    return "## {0}".format(".".join("{0:03d}".format(int(num)) for num in version.split(".")))


def get_header(version):
    """Return a Debian-compatible header (including version string)."""
    return "uhd ({0}-0ubuntu1) ubuntu_release; urgency=low\n\n".format(
        convert_version_string(version)
    )


def get_footer(uploader_name, uploader_email):
    """Return Debian-compatible changelog footer."""
    return " -- {0} <{1}>  {2}\n\n".format(
        uploader_name,
        uploader_email,
        datetime.datetime.now().strftime("%a, %d %b %Y %I:%M:%S %Z-0800"),
    )


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--input-file",
        default="CHANGELOG",
        help="Input UHD top-level changelog file",
    )
    parser.add_argument(
        "--output-file",
        default="host/cmake/debian/changelog",
        help="Output Debian changelog file (will append onto existing)",
    )
    parser.add_argument(
        "--uploader-name",
        default="Ettus Research",
        help="Uploader name (must match GPG key)",
    )
    parser.add_argument(
        "--uploader-email",
        default="uhd.maintainer@emerson.com",
        help="Uploader email (must match GPG key)",
    )
    parser.add_argument(
        "--last-version",
        help="Manually specify last version (Debian format)",
        default="",
    )
    options = parser.parse_args()

    # Input file
    with open(options.input_file, "r") as f:
        lines_in = f.readlines()

    with open(options.output_file, "r") as f:
        lines_out = f.readlines()

    if options.last_version == "":
        if len(lines_out) > 0:
            last_version = detect_last_version(lines_out[0])
            check_last_version = True
        else:
            last_version = ""
            check_last_version = False  # Will do every version
    else:
        last_version = convert_version_string(options.last_version, False)
        check_last_version = True

    new_lines_out = []
    for line in lines_in[3:]:
        if line.rstrip() == last_version and last_version != "":
            # We've fully updated, stop here
            break
        elif re.search(r"^## [0-9]{3}\.[0-9]{3}\.[0-9]{3}", line):
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

    new_lines_out += lines_out

    with open(options.output_file, "w", newline="\n") as g:
        g.writelines(new_lines_out)
