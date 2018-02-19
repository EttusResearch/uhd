#!/usr/bin/env python
#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Tool to import external dependency rpclib into UHD tree
"""

import argparse
from distutils.dir_util import copy_tree
import os
import re
import shutil
import subprocess
import tempfile

from mako.template import Template

IMPORTANT_FILES = [
    "lib",
    "include",
    "LICENSE.md",
    "README.md",
    "CHANGELOG.md"
]

REPLACE_EXPR = [
    ("RPCLIB_ASIO", "boost::asio"),
    ("\"asio.hpp\"", "<boost/asio.hpp>"),
    ("std::error_code", "boost::system::error_code"),
    ("boost::asio::strand", "boost::asio::io_service::strand"),
]


def fix_naming(workdir):
    """
    Applies all the REPLACE_EXPR in all the files
    """
    for root, dirs, files in os.walk(workdir):
        for cur_file in files:
            if re.match(".*[hc](?:c?|\.in)|inl",cur_file):
                new_file = tempfile.NamedTemporaryFile()
                with new_file as new:
                    with open(os.path.join(root,cur_file), "r") as old:
                        for line in old:
                            for repl in REPLACE_EXPR:
                                line = line.replace(*repl)
                            new.write(line)
                    new.truncate()
                    shutil.copyfile(new_file.name, os.path.join(root, cur_file))

def extract_rpclib_version(rpclib_root):
    """
    match version definition in rpclib cmakelists
    """
    with open(os.path.join(rpclib_root, "CMakeLists.txt")) as rpclib_cmake:
        version_dict = {}
        for line in rpclib_cmake:
            version_match =re.match("^.*RPCLIB_VERSION_(?P<which>MAJOR|MINOR|PATCH) (?P<number>[0-9]+).*$", line)
            if version_match:
                version_dict[version_match.group("which").lower()] = int(version_match.group("number"))
        if not set(version_dict.keys()).issuperset(set(["major", "minor", "patch"])):
            raise Exception("couldn't find right version definitions in CMakeLists.txt")
        return (version_dict["major"], version_dict["minor"], version_dict["patch"])


def copy_important_files(target_root, rpclib_root):
    """
    copy files/subdirs we consider important (see IMPORTANT_FILES)
    """
    for rpc_file in os.listdir(rpclib_root):
        if rpc_file in IMPORTANT_FILES:
            if os.path.isdir(os.path.join(rpclib_root, rpc_file)):
                copy_tree(
                    os.path.join(rpclib_root, rpc_file),
                    os.path.join(target_root, rpc_file)
                )
            else:
                shutil.copyfile(
                    os.path.join(rpclib_root, rpc_file),
                    os.path.join(target_root, rpc_file)
                )


def patch_rpclib(rpclib_root):
    """
    applies all *.patch files in the same directory as the script
    """
    curr_dir = os.path.dirname(os.path.realpath(__file__))
    for rpc_patch in (patch for patch in sorted(os.listdir(curr_dir))
                      if patch.endswith("patch")):
        print("patching with: {0}".format(rpc_patch))
        patch = open(os.path.join(curr_dir, rpc_patch), "r")
        patch_proc = subprocess.Popen(['patch', '--quiet', '-p1'], stdin=patch, cwd=rpclib_root)


def extract_rpclib(args):
    """
    THE extraction function
    """
    workdir = tempfile.mkdtemp()
    rpclib_root = args.rpclib_root
    new_rpclib_root = tempfile.mkdtemp()
    copy_tree(rpclib_root, new_rpclib_root)
    patch_rpclib(new_rpclib_root)
    copy_important_files(workdir, new_rpclib_root)
    version = extract_rpclib_version(new_rpclib_root)
    shutil.rmtree(new_rpclib_root)
    cmake_template = Template(
        filename=os.path.join(os.path.dirname(os.path.realpath(__file__)), "rpc_CMakeLists.txt"))
    real_cmake_file = cmake_template.render(
        rpclib_major_version=version[0],
        rpclib_minor_version=version[1],
        rpclib_patch_version=version[2]
    )
    with open(os.path.join(workdir, "CMakeLists.txt"), "w") as cmake_file:
        cmake_file.write(real_cmake_file)
        cmake_file.truncate()
    fix_naming(workdir)
    copy_tree(workdir, args.rpclib_target, update=True)
    shutil.rmtree(workdir)


def parse_args():
    """
    parse cmdline arguments
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("rpclib_root")
    parser.add_argument('rpclib_target')
    return parser.parse_args()


def main():
    """
    Execute this tool
    """
    args = parse_args()
    extract_rpclib(args)
    return True

if __name__ == "__main__":
    exit(not main())
