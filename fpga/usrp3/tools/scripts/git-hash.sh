#!/bin/bash

function help {
    cat <<EOHELP
Utilities to read/write the git hash for the project.

The script will attempt to get info in the following order:
- Using a git command
- Using a hash file (if speficied by user)
- Otherwise hash = 0xFFFFFFFF

Usage: $0 [--help|-h] [--write] [--hashfile=HASH_FILE]

--hashfile : Location of git hash file [project.githash]
--write    : Write the git hash to HASH_FILE in the Ettus 32-bit register format
--help     : Shows this message

EOHELP
}

hashfile="project.githash"
write=0
for arg in "$@"; do
    if [[ $arg == "--help" ]]; then
        help
        exit 0
    elif [[ $arg =~ "--hashfile="(.*) ]]; then
        hashfile=${BASH_REMATCH[1]}
    elif [[ $arg =~ "--write" ]]; then
        write=1
    fi
done

# Default hash value (failsafe)
ettus_githash32="ffffffff"

if [[ $write -eq 0 ]]; then
    git_success=0
    # First attempt: Use git
    if [[ $(command -v git) != "" ]]; then
        # Attempt to get hash from git.
        # This command will fail if we are not in a git tree
        short_hash="$(git rev-parse --verify HEAD --short=7 2>/dev/null)" && git_success=1
        if [[ $git_success -eq 1 ]]; then
            # Check if tree is clean. If yes, the top 4 bits are 0
            if (git diff --quiet 2>/dev/null); then
                ettus_githash32="0$short_hash"
            else
                ettus_githash32="f$short_hash"
            fi
        fi
    fi
    # Second attempt: Read from file if it exists
    if [[ $git_success -eq 0 ]]; then
        if [[ -f $hashfile ]]; then
            ettus_githash32=$(cat $hashfile)
        fi
    fi
    echo ${ettus_githash32}
    exit 0
else
    # Require git
    command -v git >/dev/null || { echo "ERROR: git not found"; exit 1; }
    # Get hash from git
    short_hash="$(git rev-parse --verify HEAD --short=7 2>/dev/null)" || { echo "ERROR: Not a git tree"; exit 2; }
    # Check if tree is clean. If yes, the top 4 bits are 0
    if (git diff --quiet 2>/dev/null); then
        ettus_githash32="0$short_hash"
    else
        ettus_githash32="f$short_hash"
    fi
    echo $ettus_githash32 > $hashfile
    echo "INFO: Wrote $ettus_githash32 to $hashfile"
    exit 0
fi
