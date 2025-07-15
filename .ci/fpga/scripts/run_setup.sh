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
export PATH=$HWT_SCRIPTS_DIR:$PATH
export NETWORK_LIB_BASE_PATH="/mnt/argo/RnD/eda/libraries"
# Avoid "ERROR: [Common 17-356] Failed to install all user apps."
export XILINX_LOCAL_USER_DATA=no
# Some of the FPGA tools expect TERM to be set. This avoids a bunch of error
# messages about it from being printed in the logs.
export TERM=xterm-256color

echo "---- Run hwsetup ----"
# This script sets the XILINX_VIVADO, MODELSIM, and LIB_BASE_PATH
# variables based on the agent's configuration so we can find the EDA
# tools.
pushd $BUILD_SOURCESDIRECTORY/uhddev/.ci/hwtools
source hwsetup.sh --hwtools=0.1.3
popd

echo "---- Fetch the FPGA simulation libraries ----"
# Compiling the simulation libraries.
# Extracting the modelsim.ini path from the output.
# Creating a temporary modelsim.mpf file for the scripting to work.
echo "[Library]" > modelsim.mpf
compileFpgaLibs vivado | tee compileFpgaLib.log
VIVADO_MODELSIM_INI_PATH=$(grep Reading compileFpgaLib.log | awk '{print $3}' | sed 's/\x1b\[[0-9;]*m//g')
compileFpgaLibs diamond | tee compileFpgaLib.log
DIAMOND_MODELSIM_INI_PATH=$(grep Reading compileFpgaLib.log | awk '{print $3}' | sed 's/\x1b\[[0-9;]*m//g')
# Combine all modelsim.ini files to local copy
python $(dirname $BASH_SOURCE)/merge_modelsim_ini.py \
    -o $BUILD_SOURCESDIRECTORY/modelsim.ini \
    $VIVADO_MODELSIM_INI_PATH $DIAMOND_MODELSIM_INI_PATH
# Removing the temporary files
rm -f modelsim.mpf compileFpgaLib.log

echo "---- Fetch Vivado patches ----"
export PATCHES_DIR=$BUILD_SOURCESDIRECTORY/patches
mkdir -p $PATCHES_DIR
pushd $PATCHES_DIR
# Fetch the AR76780 patch for Vivado
cp /opt/synced/Xilinx/patches/AR76780_Vivado_2021_1_preliminary_rev1.zip ./AR76780_Vivado_2021_1_preliminary_rev1.zip
unzip -q -o ./AR76780_Vivado_2021_1_preliminary_rev1.zip -d ./AR76780_Vivado_2021/
rm -f AR76780_Vivado_2021_1_preliminary_rev1.zip
# Add the AR76780 patch for the simulation library
cp /opt/synced/modelsim/patches/fir_compiler_v7_2_76780.zip ./fir_compiler_v7_2_76780.zip
unzip -q -o ./fir_compiler_v7_2_76780.zip -d ./fir_compiler_v7_2_76780/
rm -f ./fir_compiler_v7_2_76780.zip
popd

echo "---- Apply the Vivado patches ----"
pushd $BUILD_SOURCESDIRECTORY
# Apply the patch to Vivado
export XILINX_PATH=$PATCHES_DIR/AR76780_Vivado_2021/vivado
# Add the FIR compiler simulation model to local modelsim.ini
# Delete all lines containing "fir_compiler_v7_2_76780" to avoid duplicates
sed -i -e '/fir_compiler_v7_2_76780/d' ./modelsim.ini
# Duplicate line containing "fir_compiler_v7_2_16"
sed -i -e '/fir_compiler_v7_2_16/p' ./modelsim.ini
# Replace the first "fir_compiler_v7_2_16" line with our new library
PATCHES_DIR_ESC=$(echo $PATCHES_DIR | sed 's_/_\\/_g')
sed -i -e "0,/fir_compiler_v7_2_16/s/^fir_compiler_v7_2_16.*\$/fir_compiler_v7_2_76780 = $PATCHES_DIR_ESC\/fir_compiler_v7_2_76780/" ./modelsim.ini
# Tell our build tools to use this modelsim.ini for this session
export MODELSIM_INI=$(pwd)/modelsim.ini
export MSIM_VIV_COMPLIBDIR=$(pwd)
popd

echo "---- Run setupenv ----"
source $1/setupenv.sh --vivado-path $(dirname $XILINX_VIVADO) --modelsim-path $(dirname $MODELSIM)
