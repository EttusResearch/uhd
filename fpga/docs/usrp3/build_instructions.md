# Generation 3 USRP Build Documentation

## Dependencies and Requirements

### Dependencies

The USRP FPGA build system requires a UNIX-like environment with the following dependencies

- [Xilinx Vivado Design Suite HLx Editions 2019.1](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools/archive.html) (For 7 Series and UltraScale+ FPGAs)
- [Xilinx ISE Design Suite 14.7](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools/archive-ise.html) (For all other FPGAs)
- [GNU Make 3.6+](https://www.gnu.org/software/make/)
- [GNU Bash 4.0+](https://www.gnu.org/software/bash/)
- [Python 3.5](https://www.python.org/)
- [Doxygen](https://www.doxygen.nl/index.html) (Optional: To build the manual)
- [ModelSim](https://www.mentor.com/products/fv/modelsim/) (Optional: For simulation)

The following USRPs work with the free WebPACK versions:
- USRP B200/B200mini (ISE)
- USRP E310/E312/E313 (Vivado)

### What FPGA does my USRP have?

- USRP B200: Spartan-6 XC6SLX75
- USRP B210: Spartan-6 XC6SLX150
- USRP B200mini: Spartan-6 XC6SLX75
- USRP B205mini: Spartan-6 XC6SLX150
- USRP X300: Kintex-7 XC7K325T (7 Series)
- USRP X310: Kintex-7 XC7K410T (7 Series)
- USRP E310: Zynq-7000 XC7Z020 (7 Series)
- USRP E320: Zynq-7000 XC7Z045 (7 Series)
- USRP N300: Zynq-7000 XC7Z035 (7 Series)
- USRP N310/N320: Zynq-7000 XC7Z100 (7 Series)
- USRP X410: RFSoC XCZU28DR (UltraScale+)

### Requirements

- [Xilinx Vivado Release Notes](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2019_1/ug973-vivado-release-notes-install-license.pdf)
- [Xilinx ISE Platform Requirements](http://www.xilinx.com/support/documentation/sw_manuals/xilinx14_7/irn.pdf)

## Build Environment Setup

### Download and Install Xilinx Tools

Download and install Xilinx Vivado or Xilinx ISE based on the target USRP.
- The recommended installation directory is `/opt/Xilinx/` for Linux and `C:\Xilinx` in Windows.
- Please check the Xilinx requirements for the FPGA technology used by your USRP device.
- You may need to acquire a synthesis and implementation license from Xilinx to build some USRP designs.
- You may need to acquire a simulation license from Xilinx to run some testbenches.

### Download and Install ModelSim (Optional)

If you prefer to use ModelSim, download and install Mentor ModelSim using the
link above.
- The recommended installation directory is `/opt/mentor/modelsim` for Linux and `C:\mentor\modelsim` in Windows
- Supported versions are PE, DE, SE, DE-64 and SE-64
- You may need to acquire a license from Mentor Graphics to run ModelSim

### Setting up build dependencies on Ubuntu

You can install all the dependencies through the package manager:

    sudo apt-get install python bash build-essential doxygen

Your actual command may differ.

### Setting up build dependencies on Fedora

You can install all the dependencies through the package manager:

    sudo yum -y install python bash make doxygen

Your actual command may differ.

### Setting up build dependencies on Windows (using Cygwin)

**NOTE**: Windows is only supported with Vivado. The build system does not support Xilinx ISE in Windows.

Download the latest version on [Cygwin](https://cygwin.com/install.html) (64-bit is preferred on a 64-bit OS)
and install it using [these instructions](http://x.cygwin.com/docs/ug/setup-cygwin-x-installing.html).
The following additional packages are also required and can be selected in the GUI installer

    python3 patch patchutils bash make gcc-core doxygen

## Build Instructions (Xilinx Vivado only)

### Makefile based Builder

- Navigate to `<repo>/fpga/usrp3/top/{project}` where `{project}` is:
  + `x300:` For USRP X300 and USRP X310
  + `e31x:` For USRP E310
  + `e320:` For USRP E320
  + `n3xx:` For USRP N300/N310/N320
  + `x400:` For USRP X410

- To add vivado to the PATH and to setup up the Ettus Xilinx build environment run
  + `source setupenv.sh` (If Vivado is installed in the default path /opt/Xilinx/Vivado) _OR_
  + `source setupenv.sh --vivado-path=<VIVADO_PATH>` (where VIVADO_PATH is a non-default installation path)

- To build a binary configuration bitstream run `make <target>`
  where the target is specific to each product. To get a list of supported targets run
  `make help`.

- The build output will be specific to the product and will be located in the
  `<repo>/fpga/usrp3/top/{project}/build` directory. Run `make help` for more information.

### Environment Utilities

The build environment also defines many ease-of-use utilities. Please use
the \subpage md_usrp3_vivado_env_utils "Vivado Utility Reference" page for
a list and usage information

## Build Instructions (Xilinx ISE only)

### Makefile Based Builder

- To add xtclsh to the PATH and to setup up the Xilinx build environment run
  + `source <install_dir>/Xilinx/14.7/ISE_DS/settings64.sh` (64-bit platform)
  + `source <install_dir>/Xilinx/14.7/ISE_DS/settings32.sh` (32-bit platform)

- Navigate to `<repo>/fpga/usrp3/top/{project}` where `{project}` is:
  + b200: For USRP B200 and USRP B210
  + b200mini: For USRP B200mini

- To build a binary configuration bitstream run `make <target>`
  where the target is specific to each product. To get a list of supported targets run
  `make help`.

- The build output will be specific to the product and will be located in the
  `<repo>/fpga/usrp3/top/{project}/build` directory. Run `make help` for more information.

## Targets and Outputs

### B2x0 Targets and Outputs

#### Supported Targets
- B200:  Builds the USRP B200 design.
- B210:  Builds the USRP B210 design.

#### Outputs
- `build/usrp_<product>_fpga.bit` : Configuration bitstream with header
- `build/usrp_<product>_fpga.bin` : Configuration bitstream without header
- `build/usrp_<product>_fpga.syr` : Xilinx system report
- `build/usrp_<product>_fpga.twr` : Xilinx timing report

### X3x0 Targets and Outputs

#### Supported Targets
- X310_1G:  USRP X310. 1GigE on both SFP+ ports.
- X300_1G:  USRP X300. 1GigE on both SFP+ ports.
- X310_HG:  USRP X310. 1GigE on SFP+ Port0, 10Gig on SFP+ Port1.
- X300_HG:  USRP X300. 1GigE on SFP+ Port0, 10Gig on SFP+ Port1.
- X310_XG:  USRP X310. 10GigE on both SFP+ ports.
- X300_XG:  USRP X300. 10GigE on both SFP+ ports.
- X310_HA:  USRP X310. 1GigE on SFP+ Port0, Aurora on SFP+ Port1.
- X300_HA:  USRP X300. 1GigE on SFP+ Port0, Aurora on SFP+ Port1.
- X310_XA:  USRP X310. 10GigE on SFP+ Port0, Aurora on SFP+ Port1.
- X300_XA:  USRP X300. 10GigE on SFP+ Port0, Aurora on SFP+ Port1.

#### Outputs
- `build/usrp_<product>_fpga_<image_type>.bit` :    Configuration bitstream with header
- `build/usrp_<product>_fpga_<image_type>.bin` :    Configuration bitstream without header
- `build/usrp_<product>_fpga_<image_type>.lvbitx` : Configuration bitstream for PCIe (NI-RIO)
- `build/usrp_<product>_fpga_<image_type>.rpt` :    System, utilization and timing summary report

### E310 Targets and Outputs

#### Supported Targets
- E310_SG1 or E310:  Builds the USRP E310 (speed grade 1).
- E310_SG3 or E310_sg3:  Builds the USRP E310 (speed grade 3).
- E310_SG1_IDLE:  Builds the USRP E310 idle design (speed grade 1).
- E310_SG3_IDLE:  Builds the USRP E310 idle design (sSpeed grade 3).

#### Outputs
- `build/usrp_<product>_fpga.bit` : Configuration bitstream with header
- `build/usrp_<product>_fpga.dts` : Device tree overlay
- `build/usrp_<product>_fpga.rpt` : System, utilization and timing summary report

### E320 Targets and Outputs

#### Supported Targets
- E320_1G: 1GigE on SFP+ Port.
- E320_XG: 10GigE on SFP+ Port.
- E320_AA: Aurora on SFP+ Port.

#### Outputs
- `build/usrp_<product>_fpga.bit` : Configuration bitstream with header
- `build/usrp_<product>_fpga.dts` : Device tree overlay
- `build/usrp_<product>_fpga.rpt` : System, utilization and timing summary report

### N3XX Targets and Outputs

#### Supported Targets

The targets depend on the actual hardware the FPGA image is being deployed to.
Unlike the X300 Series, the daughterboards are an integral part of the module
and not meant to be removed. Therefore, the target is specific to the
combination of motherboard and daughterboards.

- N300_AA: Aurora on both SFP+ ports
- N300_HA: 1GigE on SFP0, Aurora on SFP1
- N300_HG: 1GigE on SFP0, 10GigE on SFP1
- N300_WX: White Rabbit on SFP0, 10GigE on SFP1
- N300_XA: 10GigE on SFP0, Aurora on SFP1
- N300_XG: 10GigE on both SFP+ ports
- N310_AA: Aurora on both SFP+ ports
- N310_HA: 1GigE on SFP0, Aurora on SFP1
- N310_HG: 1GigE on SFP0, 10GigE on SFP1
- N310_WX: White Rabbit on SFP0, 10GigE on SFP1
- N310_XA: 10GigE on SFP0, Aurora on SFP1
- N310_XG: 10GigE on both SFP+ ports
- N320_AQ: 10GigE on both SFP+ ports, Aurora on QSFP+ ports
- N320_HG: 1GigE on SFP0, 10GigE on SFP1
- N320_XG: 10GigE on both SFP+ ports
- N320_XQ: White Rabbit on SFP0, 10 GigE on QSFP0 and QSFP1
- N320_WX: White Rabbit on SFP0, 10GigE on SFP1
- N320_AA: Aurora on SFP+ Port0, Aurora on SFP+ Port1

For the N320 targets see also the N320 manual page on the UHD manual.

#### Outputs
- `build/usrp_<product>_fpga.bit` : Configuration bitstream with header
- `build/usrp_<product>_fpga.dts` : Device tree overlay
- `build/usrp_<product>_fpga.rpt` : System, utilization and timing summary report

### X4x0 Targets and Outputs

#### Supported Targets

Unlike the USRP X310, the target types do not only describe the connector
configuration, but also the available master clock rates. For example, the FPGA
target type `X4_200` is configured for a 200 MHz analog bandwidth, and can
support a 245.76 MHz or 250 MHz master clock rate.

A more detailed description of the targets can be found at \ref x4xx_updating_fpga_types.
Run `make help` in the `<repo>/fpga/usrp3/top/x400/` directory to see
the complete list of targets available.

#### Outputs
- `build/usrp_<product>_fpga.bit` : Configuration bitstream with header
- `build/usrp_<product>_fpga.dts` : Device tree overlay
- `build/usrp_<product>_fpga.rpt` : System, utilization and timing summary report

### Additional Build Options

It is possible to make a target and specify additional options in the form `VAR=VALUE` in
the command. For example:

    $ make X310 GUI=1

The options available are described in the following subsections.

#### Xilinx Vivado Make Options

- `GUI=1` : Run the Vivado build in GUI mode instead of batch mode. After the build is complete, Vivado provides an option to save the fully configured project for customization
- `CHECK=1` : Run elaboration only to check HDL syntax
- `SYNTH=1` : Run synthesis only
- `TOP=<module>` : Specify an alternate top-level module for syntax checking

#### Xilinx ISE Make Options

- `PROJECT_ONLY=1` : Only create a Xilinx project for the specified target(s). Useful for use with the ISE GUI.
- `EXPORT_ONLY=1` :  Export build targets from a GUI build to the build directory. Requires the project in build-\*_\* to be built.
