USRP Hardware Driver (UHD™) - Spire Version!
========================================

This is a Spire fork of the UHD project made mostly to enforce internal
versioning and build procedures.  Little, if any, internal code has been
modified.

## How to build UHD (debian package):

```
sudo apt-get install libboost1.48-all-dev
mkdir -p uhd/host/build
cd uhd/host/build
cmake ..
make
cpack
./build.sh
```

UHD is the free & open-source software driver and API for the Universal Software
Radio Peripheral (USRP™) SDR platform, created and sold by Ettus Research.

UHD supports all Ettus Research USRP™ hardware, including all motherboards and
daughterboards, and the combinations thereof.

## Documentation

The [UHD Homepage](http://code.ettus.com/redmine/ettus/projects/uhd/wiki),
through which you can find installation instructions, documentation, and further
information, is the primary source of UHD documentation and "Getting
Started"-type guides.

For technical documentation related to USRP™ hardware or UHD system
design, check out the [UHD Manual](http://files.ettus.com/manual/).

If you are looking for API documentation, check out the following resources:

* [Coding to the API](http://files.ettus.com/manual/page_coding.html)
* [Device Streaming](http://files.ettus.com/manual/page_stream.html)

Additionally, be sure to check out the Ettus Research
[FAQ](http://www.ettus.com/kb/detail/frequently-asked-questions), and the
[Knowledge Base](http://www.ettus.com/kb) for useful application notes and
tutorials.

## OS Support

UHD is primarily developed on Linux, but we also test and support the following
operating systems.

* Linux (any distribution)
* Mac OS X (PPC and Intel)
* Windows 7/Vista/XP

## Applications

UHD can be used to build stand-alone applications with USRP™ hardware, or with
third-party applications. Some common toolkits / frameworks are:

* [GNURadio](http://code.ettus.com/redmine/ettus/projects/uhd/wiki/GNU_Radio_UHD)
* [LabVIEW](http://www.ni.com/download/ni-usrp-1.3/4711/en/)
* [Simulink](http://www.mathworks.com/discovery/sdr/usrp.html)
* [OpenBTS](http://wush.net/trac/rangepublic/wiki/BuildInstallRun)
* [Iris](http://www.softwareradiosystems.com/redmine/projects/iris/wiki)
* [Redhawk](https://github.com/redhawksdr)
* [Amarisoft LTE eNodeB](http://www.amarisoft.com/?p=amarilte)

## Directories

__host/__

The source code for the user-space driver.

__firmware/__

The source code for all microprocessors in USRP hardware.

__fpga/__

The source code for the UHD FPGA images.

__images/__

This contains the package builder for FPGA and firmware images.

