#!/bin/bash
#
# Copyright 2015 Ettus Research
#

#----------------------------------------------------------------------------
# Global defaults
#----------------------------------------------------------------------------
export VIV_PLATFORM=$(uname -o)

# Vivado specific
if [[ $VIV_PLATFORM = "Cygwin" ]]; then
    if [[ -d "/cygdrive/c/Xilinx/Vivado_Lab" ]]; then
        VIVADO_BASE_PATH="/cygdrive/c/Xilinx/Vivado_Lab"
    else
        VIVADO_BASE_PATH="/cygdrive/c/Xilinx/Vivado"
    fi
    MODELSIM_BASE_PATH="/cygdrive/c/mentor/modelsim"
else
    if [[ -d "/tools/Xilinx/Vivado_Lab" ]]; then
        VIVADO_BASE_PATH="/tools/Xilinx/Vivado_Lab"
    elif [[ -d "/tools/Xilinx/Vivado" ]]; then
        VIVADO_BASE_PATH="/tools/Xilinx/Vivado"
    elif [[ -d "/opt/Xilinx/Vivado_Lab" ]]; then
        VIVADO_BASE_PATH="/opt/Xilinx/Vivado_Lab"
    else
        VIVADO_BASE_PATH="/opt/Xilinx/Vivado"
    fi
    MODELSIM_BASE_PATH="/opt/mentor/modelsim"
fi

function resolve_viv_path {
    if [[ $VIV_PLATFORM = "Cygwin" ]]; then
        echo $(cygpath -aw $1)
    else
        echo $1
    fi
}

#----------------------------------------------------------------------------
# Help message display function
#----------------------------------------------------------------------------
function setupenv_help {
    cat <<EOHELP

Usage: source setupenv.sh [--help|-h] [--vivado-path=<PATH>] [--modelsim-path=<PATH>]

--vivado-path   : Path to the base install directory for Xilinx Vivado if it is
                  not installed in one of the default search locations (e.g.,
                  /opt/Xilinx/Vivado).
--modelsim-path : Path to the base install directory for ModelSim if it is not
                  installed in one of the default search locations (e.g.,
                  /opt/mentor/modelsim). This simulation tool is optional.
--help -h       : Shows this message.

This script sets up the environment required to build FPGA images for the Ettus Research
${DISPLAY_NAME}. It will also optionally set up the the environment to run the
Modelsim simulator (although this tool is not required).

Required tools: Xilinx Vivado $VIVADO_VER (Synthesis and Simulation)
Optional tools: Mentor Graphics Modelsim (Simulation)

EOHELP
}


#----------------------------------------------------------------------------
# Validate prerequisites
#----------------------------------------------------------------------------
# Ensure required variables
if [ -z "$REPO_BASE_PATH" ]; then
    echo "ERROR: Please define the variable REPO_BASE_PATH before calling this script"
    return
fi
if [ -z "$VIVADO_VER" ]; then
    echo "ERROR: Please define the variable VIVADO_VER before calling this script"
    return
fi
if [ -z "$DISPLAY_NAME" ]; then
    echo "ERROR: Please define the variable DISPLAY_NAME before calling this script"
    return
fi
if [ ${#PRODUCT_ID_MAP[@]} -eq 0 ]; then
    echo "ERROR: Please define the variable PRODUCT_ID_MAP before calling this script"
    return
fi

# Ensure that the script is sourced directly or from another script, and not
# executed.
if [[ ${BASH_SOURCE[0]} == "$0" || ${BASH_SOURCE[1]} == "$0" ]]; then
    echo "ERROR: This script must be sourced."
    setupenv_help
    exit 1
fi

#----------------------------------------------------------------------------
# Setup and parse command line
#----------------------------------------------------------------------------
# Detect platform bitness
if [ "$(uname -m)" = "x86_64" ]; then
    BITNESS="64"
else
    BITNESS="32"
fi

# Go through cmd line options
MODELSIM_REQUESTED=0
MODELSIM_FOUND=0
PARSE_STATE=""
for i in "$@"; do
    case $i in
        -h|--help)
            setupenv_help
            return 0
            ;;
        --vivado-path=*)
            VIVADO_BASE_PATH="${i#*=}"
            PARSE_STATE=""
        ;;
        --vivado-path)
            PARSE_STATE="vivado-path"
        ;;
        --vivado-version=*)
            VIVADO_USER_VER="${i#*=}"
            PARSE_STATE=""
        ;;
        --vivado-version)
            PARSE_STATE="vivado-version"
        ;;
        --modelsim-path=*)
            MODELSIM_BASE_PATH="${i#*=}"
            MODELSIM_REQUESTED=1
            PARSE_STATE=""
        ;;
        --modelsim-path)
            PARSE_STATE="modelsim-path"
        ;;
        *)
            case $PARSE_STATE in
                vivado-path)
                    VIVADO_BASE_PATH="$i"
                    PARSE_STATE=""
                ;;
                vivado-version)
                    VIVADO_USER_VER="$i"
                    PARSE_STATE=""
                ;;
                modelsim-path)
                    MODELSIM_BASE_PATH="$i"
                    MODELSIM_REQUESTED=1
                    PARSE_STATE=""
                ;;
                *)
                    echo "ERROR: Unrecognized option: $i"
                    setupenv_help
                    return 1
                ;;
            esac
        ;;
    esac
done

# Vivado environment setup
if [[ ${VIVADO_VER^^} = "CMDLINE_ARG" ]]; then
    if [[ -z $VIVADO_USER_VER ]]; then
        echo "ERROR: The --vivado-version argument must be specified when the env version is \"CMDLINE_ARG\""
        return 1
    else
        VIVADO_VER=$VIVADO_USER_VER
    fi
fi
export VIVADO_PATH=$VIVADO_BASE_PATH/$VIVADO_VER
export VIVADO_HLS_PATH=$VIVADO_BASE_PATH/../Vitis_HLS/$VIVADO_VER

echo "Setting up a ${BITNESS}-bit FPGA build environment for the ${DISPLAY_NAME}..."
#----------------------------------------------------------------------------
# Prepare Vivado environment
#----------------------------------------------------------------------------
if [ -d "$VIVADO_PATH/bin" ]; then
    echo "- Vivado: Found ($VIVADO_PATH/bin)"
else
    echo "- Vivado: Version $VIVADO_VER not found in $VIVADO_BASE_PATH (ERROR.. Builds and simulations will not work)"
    if [[ -z $VIVADO_USER_VER ]]; then
        echo "          Use the --vivado-path option to override the search path"
    else
        echo "          Use the --vivado-path option to override the search path or specify the correct --vivado-version"
    fi
    unset VIVADO_USER_VER
    return 1
fi

$VIVADO_PATH/settings${BITNESS}.sh
$VIVADO_PATH/.settings${BITNESS}-Vivado.sh
if [[ -e $(readlink -f $VIVADO_HLS_PATH)/.settings${BITNESS}-Vitis_HLS.sh ]]; then
    $(readlink -f $VIVADO_HLS_PATH)/.settings${BITNESS}-Vitis_HLS.sh
fi
if [[ -e $(readlink -f $VIVADO_BASE_PATH/..)/DocNav/.settings${BITNESS}-DocNav.sh ]]; then
    $(readlink -f $VIVADO_BASE_PATH/..)/DocNav/.settings${BITNESS}-DocNav.sh
fi

if [[ -x `which tput 2>/dev/null` ]] ; then
    export VIV_COLOR_SCHEME=default
fi
VIVADO_EXEC="$REPO_BASE_PATH/tools/scripts/launch_vivado.sh"

# If an extended (i.e. full) version is provided, use Vivado to check
# that the installed version is correct.
# This optional env variable may be used to check for specific Vivado
# Updates or Patches to be installed.
# If multiple patches are installed, it is recommended that the user
# runs 'vivado -version' from the installation directory to determine
# the order in which patches were applied by Vivado.
# Example:
#   VIVADO_VER=2019.1
#   VIVADO_VER_FULL=2019.1.1_AR73068
# The setupenv.sh script will setup Vivado 2019.1, but it will also
# verify that both Update 1 and patch AR73068 are installed.
if [[ -n $VIVADO_VER_FULL ]]; then
    INSTALLED_VIV=$($VIVADO_PATH/bin/vivado -version | grep $VIVADO_VER_FULL)
    if [[ $? -eq 0 ]]; then
        echo "          Installed version is $INSTALLED_VIV"
    else
        echo "- Vivado: ERROR! Requested version ($VIVADO_VER_FULL) not installed."
        echo "          Install the required updates/patches and verify that the env variable"
        echo "          \"VIVADO_VER_FULL\" matches the version printed by \"vivado -version\"."
        return 1
    fi
fi

#----------------------------------------------------------------------------
# Prepare Modelsim environment
#----------------------------------------------------------------------------
if [[ -d $MODELSIM_BASE_PATH ]]; then
    if [[ $VIV_PLATFORM = "Cygwin" ]]; then
        VSIM_PATH=$(find -L $MODELSIM_BASE_PATH -maxdepth 3 -wholename '*win*/vsim.exe' | head -n 1)
    else
        VSIM_PATH=$(find -L $MODELSIM_BASE_PATH -maxdepth 3 -wholename '*linux*/vsim' | head -n 1)
    fi
fi
if [[ $VSIM_PATH ]]; then
    if [[ $($VSIM_PATH -version) =~ .*ModelSim[[:space:]](.+)[[:space:]]vsim.* ]]; then
        MODELSIM_VER=${BASH_REMATCH[1]}
        MODELSIM_PATH=$(dirname $VSIM_PATH)
    fi
    case $MODELSIM_VER in
        DE-64|SE-64)
            export MODELSIM_64BIT=1
            if [[ -z $MSIM_VIV_COMPLIBDIR ]]; then
                export SIM_COMPLIBDIR=$VIVADO_PATH/modelsim64
            else
                export SIM_COMPLIBDIR=$MSIM_VIV_COMPLIBDIR
            fi
        ;;
        DE|SE|PE)
            export MODELSIM_64BIT=0
            if [[ -z $MSIM_VIV_COMPLIBDIR ]]; then
                export SIM_COMPLIBDIR=$VIVADO_PATH/modelsim32
            else
                export SIM_COMPLIBDIR=$MSIM_VIV_COMPLIBDIR
            fi
        ;;
        *)
        ;;
    esac
    # If MSIM_MODELSIM_INI is not defined, use the modelsim.ini in the compiled
    # libraries directory. Otherwise use the one defined by MSIM_MODELSIM_INI.
    # Set MSIM_MODELSIM_INI to an empty string to use the modelsim.ini in the
    # ModelSim installation folder.
    if [[ ! -v MSIM_MODELSIM_INI ]]; then
        export MODELSIM_INI=$(resolve_viv_path $SIM_COMPLIBDIR/modelsim.ini)
    fi
fi

function build_simlibs {
    mkdir -p $SIM_COMPLIBDIR
    pushd $SIM_COMPLIBDIR
    CMD_PATH=`mktemp XXXXXXXX.vivado_simgen.tcl`
    if [[ $MODELSIM_64BIT -eq 1 ]]; then
        echo "compile_simlib -force -simulator modelsim -family all -language all -library all -directory ." > $CMD_PATH
    else
        echo "compile_simlib -force -simulator modelsim -family all -language all -library all -32bit -directory ." > $CMD_PATH
    fi
    $VIVADO_EXEC -mode batch -source $(resolve_viv_path $CMD_PATH) -nolog -nojournal
    rm -f $CMD_PATH
    popd
}

if [[ $MODELSIM_VER ]]; then
    echo "- Modelsim: Found ($MODELSIM_VER, $MODELSIM_PATH)"
    if [[ -e "$SIM_COMPLIBDIR/modelsim.ini" ]]; then
        echo "- Modelsim Compiled Libs: Found ($SIM_COMPLIBDIR)"
    else
        echo "- Modelsim Compiled Libs: Not found! (Run build_simlibs to generate them.)"
    fi
else
    if [[ $MODELSIM_REQUESTED -eq 1 ]]; then
        echo "- Modelsim: Not found in $MODELSIM_BASE_PATH (WARNING.. Simulations with vsim will not work)"
    fi
fi

#----------------------------------------------------------------------------
# Misc export variables
#----------------------------------------------------------------------------
export PATH=$(echo ${PATH} | tr ':' '\n' | awk '$0 !~ "/Vivado/"' | paste -sd:)
export PATH=${PATH}:$VIVADO_PATH:$VIVADO_PATH/bin:$VIVADO_HLS_PATH:$VIVADO_HLS_PATH/bin:$MODELSIM_PATH

for prod in "${!PRODUCT_ID_MAP[@]}"; do
    IFS='/' read -r -a prod_tokens <<< "${PRODUCT_ID_MAP[$prod]}"
    if [ ${#prod_tokens[@]} -eq 6 ]; then
        export XIL_ARCH_${prod}=${prod_tokens[0]}
        export XIL_PART_ID_${prod}=${prod_tokens[1]}/${prod_tokens[2]}/${prod_tokens[3]}/${prod_tokens[4]}/${prod_tokens[5]}
    elif [ ${#prod_tokens[@]} -eq 5 ]; then
        export XIL_ARCH_${prod}=${prod_tokens[0]}
        export XIL_PART_ID_${prod}=${prod_tokens[1]}/${prod_tokens[2]}/${prod_tokens[3]}/${prod_tokens[4]}
    elif [ ${#prod_tokens[@]} -eq 4 ]; then
        export XIL_ARCH_${prod}=${prod_tokens[0]}
        export XIL_PART_ID_${prod}=${prod_tokens[1]}/${prod_tokens[2]}/${prod_tokens[3]}
    else
        echo "ERROR: Invalid PRODUCT_ID_MAP entry: \"${PRODUCT_ID_MAP[$prod]}\". Must be <arch>/<part>/<pkg>/<sg>[/<tg>[/<rev>]]."
        return 1
    fi
done

#----------------------------------------------------------------------------
# Define IP management aliases
#----------------------------------------------------------------------------
# Vivado specific
VIV_IP_UTILS=$REPO_BASE_PATH/tools/scripts/viv_ip_utils.tcl

function viv_create_ip {
    if [[ -z $1 || -z $2 || -z $3 || -z $4 ]]; then
        echo "Create a new Vivado IP instance and a Makefile for it"
        echo ""
        echo "Usage: viv_create_new_ip <IP Name> <IP Location> <IP VLNV> <Product>"
        echo "- <IP Name>: Name of the IP instance"
        echo "- <IP Location>: Base location for IP"
        echo "- <IP VLNV>: The vendor, library, name, and version (VLNV) string for the IP as defined by Xilinx"
        echo "- <Product>: Product to generate IP for. Choose from: ${!PRODUCT_ID_MAP[@]}"
        return 1
    fi

    ip_name=$1
    ip_dir=$(readlink -f $2)
    ip_vlnv=$3
    part_name=$(python3 $REPO_BASE_PATH/tools/scripts/viv_gen_part_id.py "${PRODUCT_ID_MAP[$4]}")
    if [[ $? -ne 0 ]]; then
        echo "ERROR: Invalid product name $4. Supported: ${!PRODUCT_ID_MAP[@]}"
        return 1
    fi
    if [[ -d $ip_dir/$ip_name ]]; then
        echo "ERROR: IP $ip_dir/$ip_name already exists. Please choose a different name."
        return 1
    fi

    $VIVADO_EXEC -mode gui -source $(resolve_viv_path $VIV_IP_UTILS) -nolog -nojournal -tclargs create $part_name $ip_name $(resolve_viv_path $ip_dir) $ip_vlnv
    echo "Generating Makefile..."
    python3 $REPO_BASE_PATH/tools/scripts/viv_gen_ip_makefile.py --ip_name=$ip_name --dest=$ip_dir/$ip_name
    echo "Done generating IP in $ip_dir/$ip_name"
}

function viv_modify_ip {
    if [[ -z $1 ]]; then
        echo "Modify an existing Vivado IP instance"
        echo ""
        echo "Usage: viv_modify_ip <IP XCI Path>"
        echo "- <IP XCI Path>: Path to the IP XCI file."
        return 1
    fi

    xci_path=$(readlink -f $1)
    part_name=$(python3 $REPO_BASE_PATH/tools/scripts/viv_ip_xci_editor.py read_part $xci_path)
    if [[ -z $part_name ]]; then
        echo "ERROR: Invalid part name $part_name. XCI parse error."
        return 1
    fi
    if [[ -f $xci_path ]]; then
        $VIVADO_EXEC -mode gui -source $(resolve_viv_path $VIV_IP_UTILS) -nolog -nojournal -tclargs modify $part_name $(resolve_viv_path $xci_path)
    else
        echo "ERROR: IP $xci_path not found."
        return 1
    fi
}

function viv_modify_bd {
    if [[ -z $1 || -z $2 ]]; then
        echo "Modify an existing Vivado Block Design instance"
        echo ""
        echo "Usage: viv_modify_bd <BD Path> <Product>"
        echo "- <BD Path>: Path to the BD file."
        echo "- <Product>: Product to generate IP for. Choose from: ${!PRODUCT_ID_MAP[@]}"
        return 1
    fi

    bd_path=$(readlink -f $1)
    part_name=$(python3 $REPO_BASE_PATH/tools/scripts/viv_gen_part_id.py "${PRODUCT_ID_MAP[$2]}")
    if [[ $? -ne 0 ]]; then
        echo "ERROR: Invalid product name $2. Supported: ${!PRODUCT_ID_MAP[@]}"
        return 1
    fi
    if [[ -f $bd_path ]]; then
        $VIVADO_EXEC -mode gui -source $(resolve_viv_path $VIV_IP_UTILS) -nolog -nojournal -tclargs modify $part_name $(resolve_viv_path $bd_path)
    else
        echo "ERROR: IP $bd_path not found."
        return 1
    fi
}

function viv_modify_tcl_bd {
    if [[ -z $1 || -z $2 ]]; then
        echo "Modify an existing Vivado TCL-based Block Design instance."
        echo ""
        echo "Usage: viv_modify_bd_tcl <TCL Path> <Product>"
        echo "- <TCL Path>: Path to the TCL source file."
        echo "- <Product> : Product to generate IP for. Choose from: ${!PRODUCT_ID_MAP[@]}"
        return 1
    fi

    src_path=$(readlink -f $1)
    part_name=$(python3 $REPO_BASE_PATH/tools/scripts/viv_gen_part_id.py "${PRODUCT_ID_MAP[$2]}")
    if [[ $? -ne 0 ]]; then
        echo "ERROR: Invalid product name $2. Supported: ${!PRODUCT_ID_MAP[@]}"
        return 1
    fi
    bd_ip_repo="${src_path%/top*}/lib/vivado_ipi"
    if [[ -f $src_path ]]; then
        $VIVADO_EXEC -mode gui -source $(resolve_viv_path $VIV_IP_UTILS) -nolog -nojournal -tclargs modify_bdtcl $part_name $(resolve_viv_path $src_path) $(resolve_viv_path $bd_ip_repo)
        echo "INFO: Vivado BD was closed, writing source TCL..."
        $VIVADO_EXEC -mode batch -source $(resolve_viv_path $VIV_IP_UTILS) -nolog -nojournal -tclargs write_bdtcl $part_name $(resolve_viv_path $src_path)
    else
        echo "ERROR: IP $src_path not found."
        return 1
    fi
}

function viv_upgrade_tcl_bd {
    if [[ -z $1 || -z $2 || -z $3 ]]; then
        echo "Upgrade an existing Vivado TCL-based Block Design instance to a newer Vivado version."
        echo ""
        echo "Usage: viv_modify_bd_tcl <TCL Path> <Product> <Old Vivado Path> <Mode>"
        echo "- <TCL Path>: Path to the TCL source file."
        echo "- <Product> : Product to generate IP for. Choose from: ${!PRODUCT_ID_MAP[@]}"
        echo "- <Old Vivado Path> : Path to the old vivado executable, e.g. /opt/Xilinx/Vivado/2021.1/bin/vivado"
        echo "- <Mode> : (optional) if gui, will launch old vivado in gui mode (defaults to batch)"
        return 1
    fi

    viv_mode="batch"
    if [ "$4" == "gui" ]; then
        viv_mode="gui"
    fi

    src_path=$(readlink -f $1)
    part_name=$(python3 $REPO_BASE_PATH/tools/scripts/viv_gen_part_id.py "${PRODUCT_ID_MAP[$2]}")
    if [[ $? -ne 0 ]]; then
        echo "ERROR: Invalid product name $2. Supported: ${!PRODUCT_ID_MAP[@]}"
        return 1
    fi
    bd_ip_repo="${src_path%/top*}/lib/vivado_ipi"
    if [[ -f $src_path ]]; then
        $3 -mode $viv_mode -source $(resolve_viv_path $VIV_IP_UTILS) -nolog -nojournal -tclargs modify_bdtcl $part_name $(resolve_viv_path $src_path) $(resolve_viv_path $bd_ip_repo)
        echo "INFO: Vivado BD loaded, writing updated source TCL..."
        $VIVADO_EXEC -mode $viv_mode -source $(resolve_viv_path $VIV_IP_UTILS) -nolog -nojournal -tclargs write_bdtcl $part_name $(resolve_viv_path $src_path) upgrade
    else
        echo "ERROR: IP $src_path not found."
        return 1
    fi
}

function viv_ls_ip {
    if [[ -z $1 ]]; then
        echo "List the items in the Vivado IP catalog"
        echo ""
        echo "Usage: viv_ls_ip <Product>"
        echo "- <Product>: Product to generate IP for. Choose from: ${!PRODUCT_ID_MAP[@]}"
        return 1
    fi

    part_name=$(python3 $REPO_BASE_PATH/tools/scripts/viv_gen_part_id.py "${PRODUCT_ID_MAP[$1]}")
    if [[ $? -ne 0 ]]; then
        echo "ERROR: Invalid product name $1. Supported: ${!PRODUCT_ID_MAP[@]}"
        return 1
    fi
    $VIVADO_EXEC -mode batch -source $(resolve_viv_path $VIV_IP_UTILS) -nolog -nojournal -tclargs list $part_name | grep -v -E '(^$|^#|\*\*)'
    test ${PIPESTATUS[0]} -eq 0
}

function viv_upgrade_ip {
    if [[ -z $1 ]]; then
        echo "Upgrade one or more Xilinx IP targets"
        echo ""
        echo "Usage: viv_upgrade_ip <IP Directory> [--recursive]"
        echo "- <IP Directory>: Path to the IP XCI file."
        return 1
    fi
    max_depth="-maxdepth 1"
    if [[ $2 == "--recursive" ]]; then
        max_depth=""
    fi
    search_path=$(readlink -f $1)
    IFS='' read -r -a xci_files <<< $(find $search_path $max_depth | grep .xci | xargs)
    for xci_path in $xci_files; do
        if [[ -f $xci_path ]]; then
            echo "Upgrading $xci_path..."
            part_name=$(python3 $REPO_BASE_PATH/tools/scripts/viv_ip_xci_editor.py read_part $xci_path)
            $VIVADO_EXEC -mode batch -source $(resolve_viv_path $VIV_IP_UTILS) -nolog -nojournal -tclargs upgrade $part_name $(resolve_viv_path $xci_path) | grep -v -E '(^$|^#|\*\*)'
            test ${PIPESTATUS[0]} -eq 0
        else
            echo "ERROR: IP $xci_path not found."
            return 1
        fi
    done
}

#----------------------------------------------------------------------------
# Define hardware programming aliases
#----------------------------------------------------------------------------
VIV_HW_UTILS=$REPO_BASE_PATH/tools/scripts/viv_hardware_utils.tcl

function viv_hw_console {
    vivado -mode tcl -source $(resolve_viv_path $VIV_HW_UTILS) -nolog -nojournal
}

function viv_jtag_list {
    $VIVADO_EXEC -mode batch -source $(resolve_viv_path $VIV_HW_UTILS) -nolog -nojournal -tclargs list | grep -v -E '(^$|^#|\*\*)'
    test ${PIPESTATUS[0]} -eq 0
}

function viv_jtag_program {
    if [[ -z $1 ]]; then
        echo "Downloads a bitfile to an FPGA device using Vivado"
        echo ""
        echo "Usage: viv_jtag_program <Bitfile Path> [<FTDI Serial> = .] [<Device Address> = 0:0]"
        echo "- <Bitfile Path>: Path to a .bit FPGA configuration file"
        echo "- <FTDI Serial>: Regular expression for filtering out devices by"
        echo "                 their FTDI serial"
        echo "- <Device Address>: Address to the device in the form <Target>:<Device>"
        echo "                    Run viv_jtag_list to get a list of connected devices"
        return 1
    fi
    $VIVADO_EXEC -mode batch -source $(resolve_viv_path $VIV_HW_UTILS) -nolog -nojournal -tclargs program $* | grep -v -E '(^$|^#|\*\*)'
    test ${PIPESTATUS[0]} -eq 0
}

function probe_bitfile {
    if [[ -z $1 ]]; then
        echo "Probe a Xilinx bit file and report header information"
        echo ""
        echo "Usage: probe_bitfile <Bitfile Path>"
        echo "- <Bitfile Path>: Path to a .bit FPGA configuration file"
        return 1
    fi
    python3 $REPO_BASE_PATH/tools/scripts/xil_bitfile_parser.py --info $1
}

echo
echo "Environment successfully initialized."
return 0
