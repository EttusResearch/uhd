#!/bin/bash

#------------------------------------------
# Colorize
#------------------------------------------

# VIV_COLOR_SCHEME must be defined in the environment setup script
case "$VIV_COLOR_SCHEME" in
    default)
        CLR_OFF='tput sgr0'
        ERR_CLR='tput setaf 1'
        WARN_CLR='tput setaf 3'
        INFO_CLR='tput setaf 6'
        ;;
    *)
        CLR_OFF=''
        ERR_CLR=$CLR_OFF
        WARN_CLR=$CLR_OFF
        INFO_CLR=$CLR_OFF
esac

# Display output string colorized
function print_color {
    case $line in
        *Fatal:*|*Failure:*)
            eval $ERR_CLR; echo "$line"; eval $CLR_OFF
            ;;
        *Error:*|*Error[[:space:]]\(suppressible\):*)
            eval $ERR_CLR; echo "$line"; eval $CLR_OFF
            ;;
        *Warning:*)
            eval $WARN_CLR; echo "$line"; eval $CLR_OFF
            ;;
        *Info:*|*Note:*)
            eval $INFO_CLR; echo "$line"; eval $CLR_OFF
            ;;
        *)
            echo "$line"
    esac
}

#------------------------------------------
# Launch ModelSim
#------------------------------------------

# Using -voptargs=+acc makes everything visible in the simulator for GUI mode
# and avoids some cases where simulation mismatch could otherwise occur.
# Setting -onfinish to "stop" prevents the simulator from immediately trying to
# exit when finish() is called. This is annoying in the GUI and important for
# error detection in batch mode.
MSIM_DEFAULT="-voptargs=+acc -quiet -L unisims_ver -onfinish stop"

# DO file to execute in batch mode. This script runs the simulation and adds
# detection of error/failure assertions so that non-zero values are returned by
# ModelSim when the testbench doesn't pass. Calling std.env.finish() or
# $finish() will return 0. Detecting $fatal() requires that -onfinish be set to
# stop so the simulator doesn't quit before the DO file finishes.
DO_SCRIPT="\
quietly set BreakOnAssertion 2                           ;\
quietly set SIM_ERROR 0                                  ;\
onbreak {                                                ;\
    quietly set FINISH [lindex [runStatus -full] 2]      ;\
    if {\$FINISH eq \"unknown\"} { set SIM_ERROR 255 }   ;\
}                                                        ;\
onerror { set SIM_ERROR 255 }                            ;\
run -all                                                 ;\
quit -force -code \$SIM_ERROR                            ;\
"

# Use specified modelsim.ini, if set
if [[ -z $MSIM_MODELSIM_INI ]]; then
    MODELSIMINI_ARG=""
else
    MODELSIMINI_ARG="-modelsimini $MSIM_MODELSIM_INI"
fi

cd $MSIM_PROJ_DIR

# Generate the library options string
MSIM_LIB_ARGS=
for lib in $MSIM_LIBS
do
    MSIM_LIB_ARGS+="-L $lib "
done

if [ $MSIM_MODE == "gui" ]; then
    echo "* Launching ModelSim"
    vsim $MSIM_DEFAULT $MODELSIMINI_ARG $MSIM_ARGS $MSIM_LIB_ARGS $MSIM_SIM_TOP 2>&1 | while IFS= read -r line; do
        print_color $line
    done
    exit_status=${PIPESTATUS[0]}
    if [ ${exit_status} -ne 0 ]; then exit ${exit_status}; fi
elif [ $MSIM_MODE == "batch" ]; then
    echo "* Launching ModelSim"
    vsim -batch -do "$DO_SCRIPT" $MODELSIMINI_ARG $MSIM_DEFAULT $MSIM_ARGS $MSIM_LIB_ARGS $MSIM_SIM_TOP 2>&1 | while IFS= read -r line; do
        print_color $line
    done
    exit_status=${PIPESTATUS[0]}
    if [ ${exit_status} -ne 0 ]; then exit ${exit_status}; fi
fi
