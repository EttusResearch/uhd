#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#
#   This script first refreshes a given build IP directory to the current time
#   (assume all the IP files within that directory were just built) and then
#   the script touches all the files that changed between the current repo's
#   working tree and a given hash (used to build IP).
#   This allows the user to download pre-built IP (e.g. cached) and re-build
#   only those IP components that changed in between the cached commit and the
#   current working tree.
#
# Arguments:
#
#   $1 = absolute path to build-ip/ directory
#   $2 = git hash used to build IP in $1
#
# Example:
#
#   bash .refresh_ip.sh `realpath path/to/build-ip` 1234abc
#

# # Step 1: refresh build-ip/ directory to current time
echo "[refresh_ip.sh] Resetting timestamp for files in $1"
find $1 -type f -exec touch --no-create {} +

# # Step 2: determine all files that changed and update their time (dirty)
echo "[refresh_ip.sh] Differences between $2 and working tree (marked dirty):"
git diff --line-prefix=`git rev-parse --show-toplevel`/ --name-only $2 | \
  while read -r line; do \
    if [[ $line == *"ip/"* ]]; then \
      echo "    $line"; \
      touch --no-create $line; \
    fi \
  done
