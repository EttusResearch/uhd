#!/usr/bin/env python3

# uhd_ubuntu_deb.py
# This script is used to generate UHD dsc, changes, and source archives for upload to Launchpad.
# After dsc generation, pbuilder is called to build the debs. This sets up an envionment similar to
# the Launchpad build process. To build the dsc you must have pbuilder, debootstrap, and devscripts
# installed and have run:
#
# pbuilder create --debootstrapopts --variant=buildd --distribution=<target distro>
#
# See here for more on pbuilder: https://wiki.ubuntu.com/PbuilderHowto

import argparse
import os
import pathlib
import re
import shlex
import shutil
import subprocess
import sys
import tarfile

supported_ubuntu_releases = ["bionic", "focal", "jammy"]
tar_command = "tar --exclude='.git*' --exclude='./debian' --exclude='*.swp' --exclude='fpga' --exclude='build' --exclude='./images/*.pyc' --exclude='./images/uhd-*' --exclude='tags' --exclude='.ci' --exclude='.clang*' -cJf {}/uhd_{}.orig.tar.xz ."
debuild_command = "debuild -S -i -sa"
debuild_nosign = " -uc -us"


def main(args):
    if not pathlib.Path("host").exists():
        print("Check path. This script must be run on uhd base path")
        sys.exit(1)
    if not args.release in supported_ubuntu_releases:
        print("Unsupported release selected. Supported releases are {}".format(
            supported_ubuntu_releases))
        sys.exit(1)

    # Determine UHD version number
    uhd_version = ""
    orig_release = ""
    with open("host/cmake/debian/changelog") as cl:
        first_line = cl.readline()
        uhd_version = re.findall("[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*", first_line)
        if len(uhd_version) != 1:
            print("uhd_version in changelog malformed. Check host/cmake/debian/changelog")
            sys.exit(1)
        uhd_version = uhd_version[0]
        orig_release = re.findall("[A-Za-z_]*;", first_line)
        if len(orig_release) != 1:
            print(
                "orig_release in changelog malformed. Check host/cmake/debian/changelog")
            sys.exit(1)
        orig_release = orig_release[0].replace(";", "")

    # Compress UHD source
    if pathlib.Path(args.buildpath).exists():
        shutil.rmtree(args.buildpath)
    os.mkdir(args.buildpath)
    print("Compressing UHD Source...")
    result = subprocess.run(shlex.split(
        tar_command.format(args.buildpath, uhd_version)))
    if result.returncode:
        print("Compressing source failed")
        sys.exit(result.returncode)

    # Extract UHD source to build folder
    uhd_deb_build_path = pathlib.Path(
        args.buildpath, "uhd-{}".format(uhd_version))
    if uhd_deb_build_path.exists():
        shutil.rmtree(uhd_deb_build_path)
    with tarfile.open(args.buildpath + "/uhd_{}.orig.tar.xz".format(uhd_version), "r:xz") as uhd_archive:
        uhd_archive.extractall(path=uhd_deb_build_path)

    # Copy debian build files to build folder
    shutil.copytree("host/cmake/debian", uhd_deb_build_path / "debian")
    shutil.copy2("host/utils/uhd-usrp.rules",
                 uhd_deb_build_path / "debian/uhd-host.udev")
    with open(uhd_deb_build_path / "debian/uhd-host.manpages", "w") as man_file:
        for file in uhd_deb_build_path.glob("host/docs/*.1"):
            man_file.write(os.path.relpath(file, uhd_deb_build_path) + "\n")
        man_file.write("\n")
    for file in uhd_deb_build_path.glob("debian/*.in"):
        os.remove(file)

    # Modify changelog for selected release
    with open(uhd_deb_build_path / "debian/changelog", 'r+') as cl:
        cl_text = cl.read()
        cl_text = re.sub(orig_release, args.release, cl_text)
        cl_text = re.sub(
            "0ubuntu1", "0ubuntu1~{}1".format(args.release), cl_text)
        cl.seek(0)
        cl.write(cl_text)
        cl.truncate()

    # Generate dsc file
    result = ""
    print("Running debuild / dsc generation")
    if args.sign:
        result = subprocess.run(shlex.split(
            debuild_command), cwd=uhd_deb_build_path)
    else:
        result = subprocess.run(shlex.split(
            debuild_command + debuild_nosign), cwd=uhd_deb_build_path)
    if result.returncode:
        print("debuild / dsc generation failed")
        sys.exit(result.returncode)

    # Build debs using dsc
    if not args.nobuild:
        print("Building deb with dsc using pbuilder for {}".format(args.release))
        os.mkdir(args.buildpath + "/result")
        result = subprocess.run(shlex.split(
            "sudo pbuilder build --buildresult ./result uhd_{}-0ubuntu1~{}1.dsc".format(uhd_version, args.release)), cwd=args.buildpath)
        if result.returncode:
            print("pbuilder failed")
            sys.exit(result.returncode)

    # Upload dsc to Launchpad
    if args.upload:
        if not args.sign:
            print("Uploading requires signing. Add --sign.")
            sys.exit(1)
        result = subprocess.run(shlex.split(
            "dput ppa:ettusresearch/uhd uhd_${}-0ubuntu1~${}1_source.changes".format(uhd_version, args.release)), cwd=args.buildpath)
        if result.returncode:
            print("PPA upload failed")
            sys.exit(result.returncode)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--sign", action='store_true',
                        help="Signs files with GPG key. Not required for test builds")
    parser.add_argument("--upload", action='store_true',
                        help="Uploads to launchpad. Requires--sign")
    parser.add_argument("--nobuild", action='store_true',
                        help="Disables building using pbuilder")
    parser.add_argument("--buildpath", type=str, required=True,
                        help="Output path for build files. "
                             "Will get nuked before creating packages.")
    parser.add_argument("release", type=str,
                        help="Ubuntu release version. This must match pbuilder create --distribution if building.")
    args = parser.parse_args()
    main(args)
