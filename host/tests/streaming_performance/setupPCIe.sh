#!/bin/bash
#
# setup script for PCIe streaming performance enhancements
#
# Instructions
# copy this file to the host machine
# the script should be run as root
# sudo ./setupPCIe.sh [-v] [-s]
#
# This script configures CPU governors for optimal performance

VERBOSE=NO
STATS=NO
INSTALL_DEPS=NO
HELP=NO

while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    -h|--help)
      HELP=YES
      shift
      ;;
    -v|--verbose)
      VERBOSE=YES
      shift
      ;;
    -s|--stats)
      STATS=YES
      shift
      ;;
    --deps)
      INSTALL_DEPS=YES
      shift
      ;;
    *)
      echo "Unknown option: $1"
      echo "Use -h or --help for usage information"
      exit 1
      ;;
  esac
done

if [[ "$HELP" == "YES" ]]; then
  echo "Usage:"
  echo "  ./setupPCIe.sh [options]"
  echo ""
  echo "Setup script for PCIe streaming performance."
  echo "Configures CPU cores for performance mode."
  echo ""
  echo "Options:"
  echo "  -v, --verbose      display extra log messages"
  echo "  -s, --stats        display info about the resulting system configuration"
  echo "      --deps         installs dependencies (cpufrequtils)"
  exit 0
fi

function log() {
  if [[ "$VERBOSE" == "YES" ]]; then
    echo "$@"
  fi
}

if [[ "$INSTALL_DEPS" == "YES" ]]; then
  apt install -y cpufrequtils
  echo "Deps installed. Rerun without --deps flag."
  exit 0
fi

log ""
log "SETTING CPU GOVERNORS"
for ((i=0;i<$(nproc --all);i++)); do
  log "setting core $i to performance"
  cpufreq-set -c $i -r -g performance;
done

function print_config() {
  echo ""
  echo "CURRENT SYSTEM CONFIG"
  echo "----------------------------------------------------------------------"
  for ((i=0;i<$(nproc);i++)); do
    echo "analyzing cpu $i:"
    cpufreq-info -c $i -p
  done
}

if [[ "$STATS" == "YES" ]]; then
  print_config
fi

echo "PCIe performance setup complete."
