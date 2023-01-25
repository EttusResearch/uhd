#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#
#   Script to run NI hwsetup and UHD setupenv for the target build. Always
#   source this file.
#
#   Arguments:
#
#     $1 = Directory where setupenv is located
#
#  Example:
#
#     source run_setup.sh ./usrp3/top/x400
#

set -e

echo "---- Set environment variables ----"
export path_hwtools=$BUILD_SOURCESDIRECTORY/hwtools/head/setup
export PATH=$path_hwtools:$PATH

echo "---- Run hwsetup ----"
# This script sets the XILINX_VIVADO, MODELSIM, and LIB_BASE_PATH
# variables based on the agent's configuration so we can find the EDA
# tools.
pushd $BUILD_SOURCESDIRECTORY/uhddev/fpga/.ci/hwtools
source hwsetup.sh
popd

echo "---- Install Vivado patches ----"
mkdir -p $BUILD_SOURCESDIRECTORY/patches
pushd $BUILD_SOURCESDIRECTORY/patches
wget -q $PATCHES_PATH/2021.1/AR76780_Vivado_2021_1_preliminary_rev1.zip -O ./AR76780_Vivado_2021_1_preliminary_rev1.zip
unzip -q -o AR76780_Vivado_2021_1_preliminary_rev1.zip -d ./AR76780_Vivado_2021/
rm -f AR76780_Vivado_2021_1_preliminary_rev1.zip
export XILINX_PATH=$PWD/AR76780_Vivado_2021/vivado
wget -q $PATCHES_PATH/2021.1/fir_compiler_v7_2_76780.zip -O ./fir_compiler_v7_2_76780.zip
unzip -q -o fir_compiler_v7_2_76780.zip -d ./fir_compiler_v7_2_76780/
rm -f fir_compiler_v7_2_76780.zip
# Create a local copy of modelsim.ini and add the FIR compiler simulation model.
cp $LIB_BASE_PATH/vivado/2021.1/modelsim_SE-64_2020/modelsim.ini ./
# Delete all lines containing "fir_compiler_v7_2_76780" to avoid duplicates
sed -i -e '/fir_compiler_v7_2_76780/d' ./modelsim.ini
# Duplicate line containing "fir_compiler_v7_2_16"
sed -i -e '/fir_compiler_v7_2_16/p' ./modelsim.ini
# Replace the first "fir_compiler_v7_2_16" line with our new library
PWD_ESC=$(echo $PWD | sed 's_/_\\/_g')
sed -i -e "0,/fir_compiler_v7_2_16/s/^fir_compiler_v7_2_16.*\$/fir_compiler_v7_2_76780 = $PWD_ESC\/fir_compiler_v7_2_76780/" ./modelsim.ini
# Tell our build tools to use this modelsim.ini for this session
export MODELSIM_INI=$(pwd)/modelsim.ini
export MSIM_VIV_COMPLIBDIR=$(pwd)
popd

echo "---- Run setupenv ----"
source $1/setupenv.sh --vivado-path $(dirname $XILINX_VIVADO) --modelsim-path $(dirname $MODELSIM)
