#!/usr/bin/env python
#
# Copyright 2014 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utility module for packaging and handling UHD binary images.
"""

from __future__ import print_function
import re
import os
import sys
import hashlib

_DEFAULT_BASE_URL = "http://files.ettus.com/binaries/images"
_DEFAULT_BUFFER_SIZE      = 8192
_CMAKE_MAIN_FILE = "../host/CMakeLists.txt"
_CMAKE_IMG_TPL = """
# This section is written automatically by /images/create_imgs_package.py
# Any manual changes in here will be overwritten.
SET(UHD_IMAGES_MD5SUM "{}")
SET(UHD_IMAGES_DOWNLOAD_SRC "{}")
"""

def get_cmake_main_file():
    """ Returns a path to the CMakeLists.txt file that contains the image info. """
    return _CMAKE_MAIN_FILE

def get_base_url():
    """ Returns the base URL """
    if os.environ.get("UHD_IMAGES_BASE_URL") is not None and os.environ.get("UHD_IMAGES_BASE_URL") != "":
        return os.environ.get("UHD_IMAGES_BASE_URL")
    else:
        return _DEFAULT_BASE_URL

def base_url_is_local(base_url):
    """ Returns true if the base URL is actually a http URL
    instead of a local path. """
    return not (base_url.find('http') == 0)

def get_images_dir():
    """
    Returns the absolute position of the images/ subdir
    in the UHD source tree.
    """
    return os.path.dirname(__file__)

def update_main_cmake_file(md5, zipfilename):
    """
    Update the section in host/CMakeLists.txt that contains
    the ZIP filename and the MD5 hash.
    """
    cmakef = open(_CMAKE_MAIN_FILE, 'r').read()
    new_section = _CMAKE_IMG_TPL.format(md5, zipfilename)
    regex = re.compile("(?<={{{IMG_SECTION)(.*)(?=#}}})", flags=re.MULTILINE|re.DOTALL)
    cmakef = regex.sub(new_section, cmakef, count=1)
    open(_CMAKE_MAIN_FILE, 'w').write(cmakef)

def get_total_md5(img_dir):
    """ Creates an md5sum of everything in the images directory """
    def _update_md5_for_dir_recursive(dir_root, md5_obj):
        for (root, dirnames, filenames) in os.walk(dir_root):
            for filename in filenames:
                md5_obj.update(open(os.path.join(root, filename), 'rb').read())
                sys.stdout.write('.')
                sys.stdout.flush()
            for dirname in dirnames:
                _update_md5_for_dir_recursive(os.path.join(root, dirname), md5_obj)
    md5 = hashlib.md5()
    _update_md5_for_dir_recursive(img_dir, md5)
    print("")
    return md5.hexdigest()

def md5_checksum(filePath):
    """ Return MD5 checksum of a single file. """
    try:
        with open(filePath, 'rb') as fh:
            m = hashlib.md5()
            while True:
                data = fh.read(_DEFAULT_BUFFER_SIZE)
                if not data:
                    break
                m.update(data)
            return m.hexdigest()
    except Exception as e:
        print("Failed to calculated MD5 sum of: %s (%s)" % (filePath, e))
        raise e
