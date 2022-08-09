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
pushd $BUILD_SOURCESDIRECTORY
wget -q $PATCHES_PATH/2019.1/AR73068_Vivado_2019_1_preliminary_rev1.zip -O ./AR73068_Vivado_2019_1_preliminary_rev1.zip
unzip -q -o AR73068_Vivado_2019_1_preliminary_rev1.zip -d ./patch/
export XILINX_PATH=$PWD/patch/vivado
popd

echo "---- Run setupenv ----"
export MSIM_VIV_COMPLIBDIR=$LIB_BASE_PATH/vivado/2019.1.1/modelsim_SE-64_2020
source $1/setupenv.sh --vivado-path $(dirname $XILINX_VIVADO) --modelsim-path $(dirname $MODELSIM)
