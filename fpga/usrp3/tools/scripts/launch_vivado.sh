#!/bin/bash

#------------------------------------------
# Parse command line args
#------------------------------------------

function help {
    cat <<EOHELP

Usage: $0 [--help|-h] [--no-color] [<vivado args>]

--no-color      : Don't colorize command output
--help, -h      : Shows this message.

EOHELP
}

viv_args=""
colorize=1
for i in "$@"; do
    case $i in
        -h|--help)
            help
            exit 0
            ;;
        --no-color)
            colorize=0
        ;;
        *)
            viv_args="$viv_args $i"
        ;;
    esac
done

#------------------------------------------
# Colorize
#------------------------------------------

# VIV_COLOR_SCHEME must be defined in the environment setup script
if [ $colorize -eq 0 ]; then
    VIV_COLOR_SCHEME=none
fi

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

trim() {
    local var="$*"
    var="${var#"${var%%[![:space:]]*}"}"   # remove leading whitespace characters
    var="${var%"${var##*[![:space:]]}"}"   # remove trailing whitespace characters
    echo -n "$var"
}

VIVADO_COMMAND="vivado"
if command -v vivado_lab >/dev/null 2>&1; then
    VIVADO_COMMAND=vivado_lab
fi

$VIVADO_COMMAND $viv_args 2>&1 | while IFS= read -r line

do
    if [[ $line != \#* ]]; then # Ignore script output
        case $(trim $line) in
            *FATAL:*|*Fatal:*)
                eval $ERR_CLR; echo "$line"; eval $CLR_OFF
                ;;
            *ERROR:*|*Error:*)
                eval $ERR_CLR; echo "$line"; eval $CLR_OFF
                ;;
            *CRITICAL[[:space:]]WARNING:*|*Crtical[[:space:]]Warning:*)
                eval $CRIWARN_CLR; echo "$line"; eval $CLR_OFF
                ;;
            *WARNING:*|*Warning:*)
                eval $WARN_CLR; echo "$line"; eval $CLR_OFF
                ;;
            *)
                echo "$line"
        esac
    else
        echo "$line"
    fi
done
exit ${PIPESTATUS[0]}
