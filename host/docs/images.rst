========================================================================
UHD - Firmware and FPGA Image Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Images Overview
------------------------------------------------------------------------
Every USRP device must be loaded with special firmware and FPGA images.
The methods of loading images into the device vary among devices:

* **USRP1:** The host code will automatically load the firmware and FPGA at runtime.
* **USRP2:** The user must manually write the images onto the USRP2 SD card.
* **USRP-N Series:** The user must manually transfer the images over ethernet.
* **USRP-E Series:** The host code will automatically load the FPGA at runtime.
* **USRP-B Series:** The host code will automatically load the FPGA at runtime.

------------------------------------------------------------------------
Pre-built Images
------------------------------------------------------------------------

Pre-built images are available for download.

* `Master Branch images <http://files.ettus.com/binaries/master_images/>`_
* `Maint Branch images <http://files.ettus.com/binaries/maint_images/>`_

See the UHD wiki for the download link.

The pre-built images come in two forms:

* bundled with UHD in a platform-specific installer
* stand-alone platform-independent archive files

^^^^^^^^^^^^^^^^^^^^^^
UHD Images Downloader
^^^^^^^^^^^^^^^^^^^^^^

The UHD Images Downloader is a new feature in UHD 003.005.000. This script downloads UHD images that
are guaranteed to be compatible with the host code and places them in the default images
directory.

By default, it can be found at: **<install-path>/share/uhd/utils/uhd_images_downloader.py**

By default, it installs images to: **<install-path>/share/uhd/images**

^^^^^^^^^^^^^^^^^^^^^^
Platform installers
^^^^^^^^^^^^^^^^^^^^^^
The UNIX-based installers will install the images into **/usr/share/uhd/images**.

The Windows installers will install the images into **C:/Program Files/UHD/share/uhd/images**.

^^^^^^^^^^^^^^^^^^^^^^
Archive install
^^^^^^^^^^^^^^^^^^^^^^
When installing images from an archive, there are two options:

**Option 1:**

Unpack the archive into the UHD installation prefix.
UHD will always search **<install-path>/share/uhd/images** for image files.
Where **<install-path>** was set by the **CMAKE_INSTALL_PREFIX** at configure-time.

**Option 2:**

Unpack the archive anywhere and set the **UHD_IMAGES_PATH** environment variable.
**UHD_IMAGES_PATH** may contain a list of directories to search for image files.

------------------------------------------------------------------------
Building Images
------------------------------------------------------------------------

The UHD source repository comes with the source code necessary to build
both firmware and FPGA images for all supported devices.

The build commands for a particular image can be found in **<uhd-repo-path>/images/Makefile**.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Xilinx FPGA builds
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Xilinx ISE 12.x and up is required to build the Xilinx FPGA images.
The build requires that you have a UNIX-like environment with **Make**.
Make sure that **xtclsh** from the Xilinx ISE bin directory is in your **$PATH**.

See **<uhd-repo-path>/fpga/usrp2/top/**.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ZPU firmware builds
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The ZPU GCC compiler is required to build the ZPU firmware images.
The build requires that you have a UNIX-like environment with **CMake** and **Make**.
Make sure that **zpu-elf-gcc** is in your **$PATH**.

See **<uhd-repo-path>/firmware/zpu**.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Altera FPGA builds
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Quartus is required to build the Altera FPGA images.
Pre-built images can also be found in **<uhd-repo-path>/fpga/usrp1/rbf**.

See **<uhd-repo-path>/fpga/usrp1/toplevel/***.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
FX2 firmware builds
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The SDCC compiler is required to build the FX2 firmware images.
The build requires that you have a UNIX-like environment with **CMake** and **Make**.

See **<uhd-repo-path>/firmware/fx2**.
