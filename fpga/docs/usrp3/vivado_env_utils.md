# Vivado Environment Utilities

## Environment Setup

- Navigate to `usrp3/top/{project}` where project is:
  + x300: For USRP X300 and USRP X310
  + e3xx: For USRP E310
  + e320: For USRP E320
  + n3xx: For USRP N300/N310/N320

- To setup up the Ettus Research Xilinx build environment run
  + `source setupenv.sh` (If Vivado is installed in the default path /opt/Xilinx/Vivado) _OR_
  + `source setupenv.sh --vivado-path=<VIVADO_PATH>` (where VIVADO_PATH is a non-default installation path)

- This should not only enable building USRP FPGAs but also make the following utlities available

## ModelSim Specific

The setupenv.sh script will search the system for ModelSim installations and setup everything to run it natively and
within Vivado. The currently support versions of ModelSim are PE, DE, SE, DE-64, SE-64.

The following functions are also available in the environment:

    build_simlibs: Build ModelSim simulation libraries for Vivado

## IP Management

### Create Vivado IP

    viv_create_new_ip: Create a new Vivado IP instance and a Makefile for it

    Usage: viv_create_new_ip <IP Name> <IP Location> <IP VLNV> <Product>
    - <IP Name>: Name of the IP instance
    - <IP Location>: Base location for IP
    - <IP VLNV>: The vendor, library, name, and version (VLNV) string for the IP as defined by Xilinx
    - <Product>: Product to generate IP for

### Modify existing Vivado IP

    viv_modify_ip: Modify an existing Vivado IP instance

    Usage: viv_modify_ip <IP XCI Path>
    - <IP XCI Path>: Path to the IP XCI file.

### List supported Vivado IP

    viv_ls_ip: List the items in the Vivado IP catalog

    Usage: viv_ls_ip <Product>
    - <Product>: Product to generate IP for.

### Upgrade IP to the environment version of Vivado

    viv_upgrade_ip: Upgrade one or more Xilinx IP targets

    Usage: viv_upgrade_ip <IP Directory> [--recursive]
    - <IP Directory>: Path to the IP XCI file.

## Hardware Management

### Launch Vivado Hardware Console

    viv_hw_console: Launch the Tcl hardware console

    Usage: viv_upgrade_ip

### List connected JTAG devices

    viv_jtag_list: List all devices (and their addresses) that are connected to the system using the Xilinx platform cable

    Usage: viv_jtag_list

### Program device over JTAG

    viv_jtag_program: Downloads a bitfile to an FPGA device using Vivado

    Usage: viv_jtag_program <Bitfile Path> [<Device Address> = 0:0]
    - <Bitfile Path>: Path to a .bit FPGA configuration file
    - <Device Address>: Address to the device in the form <Target>:<Device>
                        Run viv_jtag_list to get a list of connected devices

### Probe Xilinx bitfile

    probe_bitfile: Probe a Xilinx bit file and report header information

    Usage: probe_bitfile <Bitfile Path>
    - <Bitfile Path>: Path to a .bit FPGA configuration file
