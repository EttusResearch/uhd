#!/bin/bash

#------------------------------------------
# Colorize
#------------------------------------------

# VIV_COLOR_SCHEME must be defined in the environment setup script
case "$VIV_COLOR_SCHEME" in
    default)
        CLR_OFF='tput sgr0'
        ERR_CLR='tput setaf 1'
        CRIWARN_CLR='tput setaf 1'
        WARN_CLR='tput setaf 3'
        ;;
    *)
        CLR_OFF=''
        ERR_CLR=$CLR_OFF
        CRIWARN_CLR=$CLR_OFF
        WARN_CLR=$CLR_OFF
esac

# Display output string colorized
function print_color {
    case $line in
        *Fatal:*)
            eval $ERR_CLR; echo "$line"; eval $CLR_OFF
            ;;
        *Error:*|*Error[[:space:]]\(suppressible\):*)
            eval $ERR_CLR; echo "$line"; eval $CLR_OFF
            ;;
        *Warning:*)
            eval $WARN_CLR; echo "$line"; eval $CLR_OFF
            ;;
        *)
            echo "$line"
    esac
}

#------------------------------------------
# Functions
#------------------------------------------

# Replace any directories with the files they include
function replace_dirs_with_source {
    for file in "$@"
    do
        if [ -d $file ]; then
            echo "resolve_viv_path $(realpath $(find $file -maxdepth 1 -type f)) "
        else
            echo "$file "
        fi
    done
}

#------------------------------------------
# Initialize Variables
#------------------------------------------

WORKING_DIR=$(pwd)

# Use specified modelsim.ini, if set
if [[ -z $VLINT_MODELSIM_INI ]]; then
    MODELSIMINI_ARG=""
else
    MODELSIMINI_ARG="-modelsimini $VLINT_MODELSIM_INI"
fi

# Define arguments to pass to the compile
SVLOG_ARGS="$VLINT_SVLOG_ARGS $MODELSIMINI_ARG -quiet +define+WORKING_DIR=$WORKING_DIR"
VLOG_ARGS="$VLINT_VLOG_ARGS $MODELSIMINI_ARG -quiet +define+WORKING_DIR=$WORKING_DIR"
VHDL_ARGS="$VLINT_VHDL_ARGS $MODELSIMINI_ARG -quiet"

# Define files in which to store all the compiler arguments
SV_ARGS_FILE=svlogarglist.txt
V_ARGS_FILE=vlogarglist.txt
VHD_ARGS_FILE=vcomarglist.txt

# Replace any directories with the sources they contain
SOURCES=
SOURCES+=$(replace_dirs_with_source $VLINT_INC_SRCS)
SOURCES+=$(replace_dirs_with_source $VLINT_DESIGN_SRCS)
SOURCES+=$(replace_dirs_with_source $VLINT_SIM_SRCS)

# Separate the files by type and determine include directories to use
V_FILES=
SV_FILES=
VHDL_FILES=
V_INC=
for file in $SOURCES
do
    if [[ ${file: -3} == ".sv" ]]; then
        SV_FILES+="$file "
        V_INC+="+incdir+${file%/*}/ "
    elif [[ ${file: -2} == ".v" ]]; then
        V_FILES+="$file "
        V_INC+="+incdir+${file%/*}/ "
    elif [[ ${file: -3} == ".vh" || ${file: -4} == ".svh" ]]; then
        V_INC+="+incdir+${file%/*}/ "
    elif [[ ${file: -4} == ".vhd" ]]; then
        VHD_FILES+="$file "
    fi
done

# Remove duplicates from the lists of files and directories
SV_FILES=$(printf '%s\n' $SV_FILES | awk '!a[$0]++')
V_FILES=$(printf '%s\n' $V_FILES | awk '!a[$0]++')
VHD_FILES=$(printf '%s\n' $VHD_FILES | awk '!a[$0]++')
V_INC=$(printf '%s\n' $V_INC | awk '!a[$0]++')

#------------------------------------------
# Compile HDL
#------------------------------------------

# Generate argument files
mkdir -p ./$VLINT_PROJ_DIR
cd ./$VLINT_PROJ_DIR

echo "/* Auto generated argument file for vlog -sv */" > $SV_ARGS_FILE
echo "-sv" >> $SV_ARGS_FILE
printf '%s\n'  $V_INC $SV_FILES >> $SV_ARGS_FILE

echo "/* Auto generated argument file for vlog -v */" > $V_ARGS_FILE
echo "-vlog01compat" >> $V_ARGS_FILE
printf '%s\n' $V_INC $V_FILES >> $V_ARGS_FILE

echo "/* Auto generated argument file for vcom */" > $VHD_ARGS_FILE
echo "-2008" >> $VHD_ARGS_FILE
printf '%s\n' $VHD_FILES >> $VHD_ARGS_FILE

# Run ModelSim compiler for each file type
if [[ -n "$SV_FILES" ]]; then
    echo "* Compiling SystemVerilog"
    vlog $SVLOG_ARGS -sv -f svlogarglist.txt 2>&1 | while IFS= read -r line; do
        print_color $line
    done
    exit_status=${PIPESTATUS[0]}
    if [ ${exit_status} -ne 0 ]; then exit ${exit_status}; fi
fi
if [[ -n "$V_FILES" ]]; then
    echo "* Compiling Verilog"
    vlog $VLOG_ARGS -f vlogarglist.txt 2>&1 | while IFS= read -r line; do
        print_color $line
    done
    exit_status=${PIPESTATUS[0]}
    if [ ${exit_status} -ne 0 ]; then exit ${exit_status}; fi
fi
if [[ -n "$VHD_FILES" ]]; then
    echo "* Compiling VHDL"
    vcom $VHDL_ARGS -f vcomarglist.txt 2>&1 | while IFS= read -r line; do
        print_color $line
    done
    exit_status=${PIPESTATUS[0]}
    if [ ${exit_status} -ne 0 ]; then exit ${exit_status}; fi
fi




