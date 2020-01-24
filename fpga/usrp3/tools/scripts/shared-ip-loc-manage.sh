#!/bin/bash

function help {
    cat <<EOHELP

Usage: shared-ip-loc-manage.sh [--help|-h] [--timeout=<TIMEOUT>] --path=<PATH> <ACTION>

--path      : Path to IP location
--timeout   : Timeout in seconds for the operation to complete [Optional]
              (Default: 1200)
--force     : Force operation
--help -h   : Shows this message.

ACTION      : Choose from reserve, release

EOHELP
}

function wait_for_lock {
    if [ -d "$ip_dir" ]; then
        remaining=$(($timeout))
	    trap 'echo"";echo "BUILDER: Waiting for concurrent IP build to finish... (skipped)";break;' SIGINT; \
        while [ -f "$ip_dir/.build_lock" ]; do
            if [ $remaining -gt 0 ]; then 
                echo -ne "Waiting for concurrent IP build to finish... (${remaining}s [Ctrl-C to proceed])\033[0K\r"
                sleep 1
                : $((remaining--))
            else
                break
            fi
        done
	    trap - SIGINT; \
        if [ $remaining -eq 0 ]; then
            echo "BUILDER: Waiting for concurrent IP build to finish... (timeout)"
        fi
    fi
}

function lock {
    if [ -d "$ip_dir" ]; then
        touch $ip_dir/.build_lock
    fi
}

function unlock {
    rm -f $ip_dir/.build_lock
}

function reserve {
    if [ -d "$ip_dir" ]; then
        wait_for_lock
        if [ $remaining -eq 0 ]; then
            echo "Force creating new IP location: $ip_dir"
            unlock
            rm -rf $ip_dir
            mkdir -p $ip_dir
        fi
    fi    
    if [ ! -d "$ip_dir" ]; then
        mkdir -p $ip_dir
    fi
    echo "BUILDER: Reserving IP location: $ip_dir"
    lock
}

function release {
    echo "BUILDER: Releasing IP location: $ip_dir"
    unlock
}

# Parse options
ip_dir=""
action=""
timeout=1800
remaining=0
force=0

for arg in "$@"; do
    if [[ $arg == "--help" ]]; then
        help
        exit 0
    elif [[ $arg == "--force" ]]; then
        force=1
    elif [[ $arg =~ "--path="(.+) ]]; then
        ip_dir=`readlink -m ${BASH_REMATCH[1]}`
    elif [[ $arg =~ "--timeout="(.+) ]]; then
        timeout=${BASH_REMATCH[1]}
    else
        action=$arg
        break
    fi
done

# Validate inputs
if [ -z $ip_dir ]; then
    echo "ERROR: Please specify a valid path using the --path option."
    exit 1
fi

case $action in
    reserve)
        if [ $force -eq 1 ]; then
            echo "Force creating new IP location: $ip_dir"
            rm -rf $ip_dir
            mkdir -p $ip_dir
            lock
        else
            reserve
        fi
        ;;
    release)
        release
        ;;
    *)
        echo "ERROR: Please specify a valid action (reserve, release)"
        exit 1
        ;;
esac

