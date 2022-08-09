#!/bin/bash
#
# setup script for streaming performance enhancements
#
# Instructions
# copy this file to the host machine
# the script should be run as root
# sudo ./setup.sh <$sfp0> <$sfp1> ... <$sfpn> [-v] [-s]
# sudo ./setup.sh enp1s0f0 enp1s0f1
# sudo ./setup.sh --auto
# sudo ./setup.sh --deps
#
# limitations:
# - does not cover thread priority scheduling
# more info: https://kb.ettus.com/USRP_Host_Performance_Tuning_Tips_and_Tricks#Thread_Priority_Scheduling
# - does not write grub files for using dpdk
# more info: https://files.ettus.com/manual/page_dpdk.html#dpdk_system_configuration
# - assumes system has Ubuntu 18.04/DPDK 17.11, and may not work for other distributions

ETH_10_GIGS=()
DPDK_DEVS=()
AUTO=NO
VERBOSE=NO
STATS=NO
INSTALL_DEPS=NO
USE_DPDK=NO
CURRENT_STATE=NO
MAC_ADDRS=()
GEN_CONF=NO
FORCE_OVERWRITE_CONF=NO
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
    --auto)
      AUTO=YES
      shift
      ;;
    --deps)
      INSTALL_DEPS=YES
      shift
      ;;
    --dpdk)
      USE_DPDK=YES
      shift
      ;;
    --gen)
      GEN_CONF=YES
      shift
      ;;
    --force)
      FORCE_OVERWRITE_CONF=YES
      shift
      ;;
    *)
      ETH_10_GIGS+=($1)
      shift
      ;;
  esac
done

if [[ "$HELP" == "YES" ]]; then
  echo "Usage:"
  echo "  ./setup.sh [options] <interface0> <interface1> ..."
  echo ""
  echo "where the interfaces are 10 GigE network interfaces."
  echo "these can be specified by name, or by using the --auto flag."
  echo ""
  echo "Options:"
  echo "  -v, --verbose      display extra log messages"
  echo "  -s, --stats        display info about the resulting system configuration"
  echo "      --auto         searches for available SFP interfaces instead of the user specifying them"
  echo "      --deps         installs dependencies for uhd, rfnoc, dpdk, and utils"
  echo "      --dpdk         sets up the system for using dpdk"
  echo "      --gen          generates a uhd conf file template for the system interfaces"
  echo "      --force        loads uhd conf file in the current directory as the system-wide uhd.conf"
  exit 0
fi

function log() {
  if [[ "$VERBOSE" == "YES" ]]; then
    echo "$@"
  fi
}

if [[ "$INSTALL_DEPS" == "YES" ]]; then
  apt install -y cpufrequtils ethtool net-tools
  apt install -y dpdk dpdk-dev
  echo "Deps installed. Rerun without --deps flag."
  exit 0
fi

function detect_mode() {
  if [[ -n "$(dpdk-devbind -s | grep 'drv=vfio-pci')" ]]; then
    CURRENT_STATE=DPDK
  else
    CURRENT_STATE=DEFAULT
  fi
}

function reset_state() {
  detect_mode
  if [[ "$CURRENT_STATE" == "DPDK" ]]; then
    log ""
    log "RESETTING CURRENT DPDK STATE"
  fi
  while [[ "$CURRENT_STATE" == "DPDK" ]]; do
    dev_info="$(dpdk-devbind -s | grep 'drv=vfio-pci')"
    log "found dpdk dev:$dev_info"
    dev_name=${dev_info%% \'*}
    dev_drv=${dev_info#*unused=}
    dev_drv=${dev_drv%%[$'\n']*}
    log "binding $dev_name to $dev_drv"
    sudo dpdk-devbind --bind=$dev_drv $dev_name
    detect_mode
  done
}

function get_devs() {
  if [[ ${#ETH_10_GIGS[@]} -ne 0 ]]; then
    echo "Rerun with only either --auto flag or network interfaces specified in arguments."
    exit 1
  fi
  dev="$(dpdk-devbind -s | grep 'SFP')"
  for dev_info in $dev; do
    if [[ "$dev_info" =~ if=.* ]]; then
      iface="${dev_info#if=}"
      ETH_10_GIGS+=($iface)
      log "found network iface: $iface"
    fi
  done
}

function get_mac_addrs() {
  for iface in "${ETH_10_GIGS[@]}"; do
    mac_addr="$(ifconfig $iface | grep 'ether ')"
    mac_addr=${mac_addr#*ether }
    mac_addr=${mac_addr% txqueuelen*}
    MAC_ADDRS+=($mac_addr)
    log "found mac address: $mac_addr"
  done
}

detect_mode
reset_state
log ""
log "SEARCHING FOR DEVICE INFO"
if [[ "$AUTO" == "YES" ]]; then
  get_devs
fi
if [[ ${#ETH_10_GIGS[@]} -eq 0 ]]; then
  echo "[warning] no 10 GigE Interfaces specified or found."
fi
get_mac_addrs

log ""
log "SETTING CPU GOVERNORS"
for ((i=0;i<$(nproc --all);i++)); do
  log "setting core $i to performance"
  cpufreq-set -c $i -r -g performance;
done

log ""
log "SETTING NETWORK BUFFERS"
sysctl -w net.core.rmem_max=625000000
sysctl -w net.core.wmem_max=625000000
sysctl -w net.core.rmem_default=625000000
sysctl -w net.core.wmem_default=625000000

function set_network_iface() {
  log ""
  log "SETTING UP NETWORK INTERFACE"
  if [[ "$1" != "NO" ]]; then
    ifconfig $1 up
    log "setting $1 mtu 9000"
    ifconfig $1 mtu 9000
    log "setting $1 ring buffer: rx 4096 tx 4096"
    ethtool -G $1 rx 4096 tx 4096
  fi
}

if [[ "$USE_DPDK" == "NO" ]]; then
  for iface in "${ETH_10_GIGS[@]}"; do
    set_network_iface $iface
  done
fi

if [[ "$USE_DPDK" == "YES" ]]; then
  log ""
  log "SETTING UP DPDK"
  modprobe vfio-pci
  for iface in "${ETH_10_GIGS[@]}"; do
    ifconfig $iface down
    log "binding $iface to vfio-pci"
    sudo dpdk-devbind --bind=vfio-pci $iface
  done
fi

function generate_uhd_conf() {
  current_dir=`pwd`
  file_path=$current_dir/uhd.conf
  log "creating template file: $file_path"
  echo ";This is a generated template for uhd.conf" > $file_path
  echo ";To apply these changes replace /etc/uhd/uhd.conf with this file." >> $file_path
  echo ";Users will likely want to change some of these values to better" >> $file_path
  echo ";suit their individual system." >> $file_path
  echo ";more info here: https://files.ettus.com/manual/page_dpdk.html#dpdk_nic_config" >> $file_path
  echo "[use_dpdk=1]" >> $file_path
  echo "dpdk-mtu=9000" >> $file_path
  echo "dpdk-corelist=0,1" >> $file_path
  echo "dpdk_num_mbufs=4096" >> $file_path
  echo "dpdk_mbuf_cache_size=64" >> $file_path
  echo "" >> $file_path
  subnet=$((10))
  for mac_addr in ${MAC_ADDRS[@]}; do
    echo "[dpdk_mac=$mac_addr]" >> $file_path
    echo "dpdk_lcore = 1" >> $file_path
    echo "dpdk_ipv4 = 192.168.$subnet.1/24" >> $file_path
    subnet=$(($subnet+10))
    if [[ "$mac_addr" == "${MAC_ADDRS[0]}" ]]; then
      echo "dpdk_num_desc=4096" >> $file_path
    fi
    echo "" >> $file_path
  done
}

if [[ "$GEN_CONF" == "YES" ]]; then
  log ""
  log "GENERATING UHD.CONF TEMPLATE FILE"
  generate_uhd_conf
fi

function overwrite_uhd_conf() {
  current_dir=`pwd`
  template_file_path=$current_dir/uhd.conf
  target_file_path=/etc/uhd/
  log "copying $template_file_path to $target_file_path"
  if [[ ! -e $target_file_path ]]; then
    mkdir $target_file_path
  fi
  cp $template_file_path $target_file_path
}

if [[ "$FORCE_OVERWRITE_CONF" == "YES" ]]; then
  log ""
  log "OVERWRITING SYSTEM UHD.CONF FILE"
  overwrite_uhd_conf
fi

function print_config() {
  echo ""
  echo "CURRENT SYSTEM CONFIG"
  echo "----------------------------------------------------------------------"
  sysctl net.core.rmem_default
  sysctl net.core.wmem_default
  sysctl net.core.rmem_max
  sysctl net.core.wmem_max
  for ((i=0;i<$(nproc);i++)); do
    echo "analyzing cpu $i:"
    cpufreq-info -c $i -p
  done
  if [[ "$USE_DPDK" == "NO" ]]; then
    for iface in "${ETH_10_GIGS[@]}"; do
      ifconfig $iface
      ethtool -g $iface
    done
  else
    echo "network devices using DPDK-compatible driver:"
    dpdk-devbind -s | grep 'drv=vfio-pci'
  fi
}

if [[ "$STATS" == "YES" ]]; then
  print_config
fi
