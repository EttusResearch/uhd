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
source hwsetup.sh
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

echo "---- Configure modelsim paths ----"
# Tell our build tools to use this modelsim.ini for this session
export MODELSIM_INI=$BUILD_SOURCESDIRECTORY/modelsim.ini
export MSIM_VIV_COMPLIBDIR=$BUILD_SOURCESDIRECTORY

echo "---- Run setupenv ----"
# update XILINX_VIVADO to the parent directory
echo "Sourcing $1/setupenv.sh with Vivado path: $XILINX_VIVADO and Modelsim path: $(dirname $MODELSIM)"
source $1/setupenv.sh --vivado-path $XILINX_VIVADO --modelsim-path $(dirname $MODELSIM)
