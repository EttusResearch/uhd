USRP Hardware Driver (UHD™) Software
========================================

Welcome to the UHD™ software distribution! UHD is the free & open-source
software driver and API for the Universal Software Radio Peripheral (USRP™) SDR
platform, created and sold by Ettus Research.

UHD supports all Ettus Research USRP™ hardware, including all motherboards and
daughterboards, and the combinations thereof.

## Documentation

For technical documentation related to USRP™ hardware or UHD system
design, check out the [UHD and USRP Manual](http://files.ettus.com/manual/).
That is where you can find
[Installation Instructions](http://files.ettus.com/manual/page_install.html),
help on how to
[build UHD from source](http://files.ettus.com/manual/page_build_guide.html) on
different platforms, development guidelines and reference documentation as well
as device usage guidance.

Additionally, be sure to check out the Ettus Research
[FAQ](http://www.ettus.com/kb/detail/frequently-asked-questions), and the
[Knowledge Base](http://www.ettus.com/kb) for useful application notes and
tutorials.

## OS Support

UHD is primarily developed on Linux, but we also test and support the following
operating systems.

* Linux (Fedora and Ubuntu)
* Mac OS X (Intel)
* Windows 7

Other operating systems will most likely work, too, but are not officially
supported.

## Applications

UHD can be used to build stand-alone applications with USRP™ hardware, or with
third-party applications. Some common toolkits / frameworks are:

* [GNU Radio](http://gnuradio.org/)
* [NI LabVIEW](http://www.ni.com/download/ni-usrp-1.3/4711/en/)
* [MathWorks Simulink](http://www.mathworks.com/discovery/sdr/usrp.html)
* [REDHAWK](http://redhawksdr.github.io/Documentation/)
* [OpenBTS GSM](http://openbts.org)
* [Osmocom GSM](http://osmocom.org)
* [Amarisoft LTE](http://www.amarisoft.com/products-lte-ue-ots-sdr-pcie)
* [Software Radio Systems LTE](http://www.softwareradiosystems.com/products)

## Directories

__host/__

The source code for the user-space driver.

__firmware/__

The source code for all microprocessors in USRP hardware.

__fpga-src/__

The source code for the UHD FPGA images. Note this is a git submodule,
if you are cloning the repository and want to modify the FPGA code,
you will need to run 'git clone --recursive' to automatically
populate this directory. Alternatively, you can run 'git submodule init'
followed by 'git submodule update' to populate it after cloning the
repository without '--recursive'.

Note that this subdirectory is very large, and not necessary for
building applications that link against UHD.

__images/__

This contains the package builder for FPGA and firmware images.
We provide other tools to downloade image packages, the scripts in here
are mainly relevant for UHD maintainers and -developers.

__tools/__

Additional tools, mainly for debugging purposes. See the readme-file
in that directory for more details on the individual tools.

