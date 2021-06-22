#!/usr/bin/python3
"""
Simple program to remove all comments from pybind11 files.
It will retain the copyright header.
"""
import re
import os

# Note: This is the default header of the PyBind11 files, not of this file!
DEFAULT_HEADER="""/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/
"""

def comment_remover(text):
    """
    From:
    https://stackoverflow.com/questions/241327/python-snippet-to-remove-c-and-c-comments
    """
    def replacer(match):
        s = match.group(0)
        if s.startswith('/'):
            return " " # note: a space and not an empty string
        else:
            return s
    pattern = re.compile(
        r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
        re.DOTALL | re.MULTILINE
    )
    return re.sub(pattern, replacer, text)

def remove_trailing_whitespace(text):
    """ Remove, uh, trailing whitespace. """
    pattern = re.compile(r' +$', re.DOTALL | re.MULTILINE)
    return re.sub(pattern, '', text)


def main():
    """ Go Go Go! """
    for root, _, files in os.walk("include/pybind11"):
        for fname in files:
            path = os.path.join(root, fname)
            print("Modifying {}...".format(path))
            new_file = comment_remover(open(os.path.join(root, fname)).read())
            new_file = remove_trailing_whitespace(new_file)
            open(os.path.join(root, fname), 'w').write(
                DEFAULT_HEADER + new_file)

if __name__ == "__main__":
    main()
