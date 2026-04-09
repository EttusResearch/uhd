#!/bin/bash

VIVADO_VER=2021.1
VIVADO_VER_FULL=2021.1_AR76780
DISPLAY_NAME="USRP-B310"
REPO_BASE_PATH=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)

declare -A PRODUCT_ID_MAP
PRODUCT_ID_MAP["B310"]="kintex7/xc7k325t/fbg676/-1"

# Set default part for simulation
export ARCH=kintex7
export PART_ID="xc7k325t/fbg676/-1"

source $REPO_BASE_PATH/tools/scripts/setupenv_base.sh
