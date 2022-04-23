#!/bin/sh
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Shell script to generate uhd tar.gz packages.
# Builds 2 packages. One with the firmware, fpga, and mpm folders and one without

set -e

echo_help () {
    echo "-h/--help: This help doc"
    echo "-d/--dir /path/to/uhd/root: Point to custom path. Defaults to '../'"
    echo "-n/--name Specifies package name. Required"
    echo "-f/--fpganame Specifies fpga package name. If unspecified, '_fpga' gets appended to provided name"
}

while [ $# -gt 0 ]
do
key="$1"

case $key in
    -h|--help)
        echo_help
        exit 0
        ;;
    -d|--dir)
        UHD_ROOTDIR="$2"
        shift # past argument
        shift # past value
        ;;
    -n|--name)
        UHD_TARNAME="$2"
        shift # past argument
        shift # past value
        ;;
    -f|--fpganame)
        UHD_TARFPGANAME="$2"
        shift # past argument
        shift # past value
        ;;
    *)    # unknown option
        echo "Unknown argument"
        exit 1
        ;;
esac
done

if [ -z "$UHD_ROOTDIR" ]; then
    UHD_ROOTDIR='..'
fi
echo "Root Dir: "$UHD_ROOTDIR

if [ -z "$UHD_TARNAME" ]; then
    echo_help
    exit 1
fi

if [ -z "$UHD_TARFPGANAME" ]; then
    UHD_TARFPGANAME="${UHD_TARNAME}_fpga"
fi

# Create working directory if it doesn't already exist otherwise clear it
if [ ! -d "$UHD_ROOTDIR/build/root" ]; then
    mkdir -p $UHD_ROOTDIR/build/root
else
    rm -r $UHD_ROOTDIR/build/root/*
fi

# Copy basic source and create tar.gz
cp -R $UHD_ROOTDIR/host $UHD_ROOTDIR/build/root
cp -R $UHD_ROOTDIR/images $UHD_ROOTDIR/build/root
cp -R $UHD_ROOTDIR/tools $UHD_ROOTDIR/build/root
find $UHD_ROOTDIR/build/root/ -type d -name "build*" -exec rm -r {} +

UHD_EXITSTATUS=0

echo "Building $UHD_ROOTDIR/build/$UHD_TARNAME.tar.bz2"
tar --exclude='.git*' -jcf $UHD_ROOTDIR/build/$UHD_TARNAME.tar.bz2 -C $UHD_ROOTDIR/build/root/ . || { echo "Could not create .tar.bz2 file -- Is bzip2 installed?"; UHD_EXITSTATUS=1; }

echo "Building $UHD_ROOTDIR/build/$UHD_TARNAME.tar.gz"
tar --exclude='.git*' -zcf $UHD_ROOTDIR/build/$UHD_TARNAME.tar.gz -C $UHD_ROOTDIR/build/root/ . || { echo "Could not create .tar.gz file -- Is gzip installed?"; UHD_EXITSTATUS=1; }

echo "Building $UHD_ROOTDIR/build/$UHD_TARNAME.tar.xz"
tar --exclude='.git*' -Jcf $UHD_ROOTDIR/build/$UHD_TARNAME.tar.xz -C $UHD_ROOTDIR/build/root/ . || { echo "Could not create .tar.xz file -- Is xz installed?"; UHD_EXITSTATUS=1; }

echo "Building $UHD_ROOTDIR/build/$UHD_TARNAME.tar.Z"
tar --exclude='.git*' -Zcf $UHD_ROOTDIR/build/$UHD_TARNAME.tar.Z -C $UHD_ROOTDIR/build/root/ . || { echo "Could not create .tar.Z file -- Is compress installed?"; UHD_EXITSTATUS=1; }

# Copy firmware, fpga, and mpm folders and create tar.gz
cp -R $UHD_ROOTDIR/firmware $UHD_ROOTDIR/build/root
cp -R $UHD_ROOTDIR/fpga $UHD_ROOTDIR/build/root
cp -R $UHD_ROOTDIR/mpm $UHD_ROOTDIR/build/root
find $UHD_ROOTDIR/build/root/ -type d -name "build*" -exec rm -r {} +

echo "Building $UHD_ROOTDIR/build/$UHD_TARFPGANAME.tar.bz2"
tar --exclude='.git*' -jcf  $UHD_ROOTDIR/build/$UHD_TARFPGANAME.tar.bz2 -C $UHD_ROOTDIR/build/root/ . || { echo "Could not create .tar.bz2 file -- Is bzip2 installed?"; UHD_EXITSTATUS=1; }

echo "Building $UHD_ROOTDIR/build/$UHD_TARFPGANAME.tar.gz"
tar --exclude='.git*' -zcf $UHD_ROOTDIR/build/$UHD_TARFPGANAME.tar.gz -C $UHD_ROOTDIR/build/root/ . || { echo "Could not create .tar.gz file -- Is gzip installed?"; UHD_EXITSTATUS=1; }

echo "Building $UHD_ROOTDIR/build/$UHD_TARFPGANAME.tar.xz"
tar --exclude='.git*' -Jcf $UHD_ROOTDIR/build/$UHD_TARFPGANAME.tar.xz -C $UHD_ROOTDIR/build/root/ . || { echo "Could not create .tar.xz file -- Is xz installed?"; UHD_EXITSTATUS=1; }

echo "Building $UHD_ROOTDIR/build/$UHD_TARFPGANAME.tar.Z"
tar --exclude='.git*' -Zcf $UHD_ROOTDIR/build/$UHD_TARFPGANAME.tar.Z -C $UHD_ROOTDIR/build/root/ . || { echo "Could not create .tar.Z file -- Is compress installed?"; UHD_EXITSTATUS=1; }

exit $UHD_EXITSTATUS
