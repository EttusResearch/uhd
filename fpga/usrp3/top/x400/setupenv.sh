#!/bin/bash
#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

VIVADO_VER=2021.1
VIVADO_VER_FULL=2021.1_AR76780
DISPLAY_NAME="USRP-X4xx"
REPO_BASE_PATH=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)

declare -A PRODUCT_ID_MAP
PRODUCT_ID_MAP["X410"]="zynquplusRFSOC/xczu28dr/ffvg1517/-1/e"

# Set default part for simulation
export ARCH=zynquplusRFSOC
export PART_ID="xczu28dr/ffvg1517/-1/e"

source $REPO_BASE_PATH/tools/scripts/setupenv_base.sh
