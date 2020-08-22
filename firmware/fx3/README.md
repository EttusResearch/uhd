INSTRUCTIONS
================================

# Building the B2xx FX3 Firmware

The USRP B200 and B210 each use the Cypress FX3 USB3 PHY for USB3 connectivity.
This device has an ARM core on it, which is programmed in C. This README will
show you how to build our firmware and bootloader source.

**A brief "Theory of Operations":**
The host sends commands to the FX3, our USB3 PHY, which has an on-board ARM
which runs the FX3 firmware code (hex file). That code is responsible for
managing the transport from the host to the FPGA by configuring IO and DMA.

## Setting up the Cypress SDK

In order to compile the USRP B200 and B210 bootloader and firmware, you will
need the FX3 SDK distributed by Cypress Semiconductor.  You can download the
[FX3 SDK from here](https://www.cypress.com/documentation/software-and-drivers/ez-usb-fx3-software-development-kit)

From the SDK, extract the ARM cross-compile toolchain tarball `ARM-GCC.tar.gz`
and the FX3 firmware tarball `fx3_firmware_linux.tar.gz`.

Set up the appropriate environment variables for the build:

```
    $ export ARMGCC_INSTALL_PATH=<ARM toolchain dir>
    $ export ARMGCC_VERSION=<ARM toolchain version>
    $ export FX3FWROOT=<FX3 firmware dir>
```

These should look something like this:

```
    $ export ARMGCC_INSTALL_PATH=/path/to/my/files/arm-2013.11
    $ export ARMGCC_VERSION=4.8.1
    $ export FX3FWROOT=/path/to/my/files/cyfx3sdk
```

## Applying the Patch to the Toolchain

Now, you'll need to apply a patch to some files in the Cypress SDK. Apply the
patch `b200/fx3_mem_map.patch` as follows:

```
    $ cd uhd.git/firmware/fx3
    $ patch -p1 -d $FX3FWROOT < b200/fx3_mem_map.patch
```

If you don't see any errors print on the screen, then the patch was successful.

## Building the Firmware

Now, you should be able to head into the `b200/firmware` directory and simply
build the firmware:

```
    $ cd uhd.git/firmware/fx3/b200/firmware
    $ make
```

It will generate a `usrp_b200_fw.hex` file, which you can then give to UHD to
program your USRP B200 or USRP B210.

## Building the Bootloader

The bootloader is built in the `b200/bootloader` directory:

```
    $ cd uhd.git/firmware/fx3/b200/bootloader
    $ make
```

It will generate a `usrp_b200_bl.img` file, which you can supply as an argument
to b2xx_fx3_utils to load it onto the device.
