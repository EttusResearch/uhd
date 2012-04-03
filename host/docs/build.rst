========================================================================
UHD - Build Guide
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Build Dependencies
------------------------------------------------------------------------

**Linux Notes:**
This is dependent on the distribution you are using, but most, if not all, of
the dependencies should be available in the package repositories for your
package manager.

**Mac OS X Notes:**
Install the "Xcode Developer Tools" to get the build tools (GCC and Make).
Use MacPorts to get the Boost and Cheetah dependencies.
Other dependencies can be downloaded as DMG installers from the web.

**Windows Notes:**
The dependencies can be acquired through installable EXE files.
Usually, the Windows installer can be found on the project's website.
Some projects do not host Windows installers, and if this is the case,
follow the auxiliary download URL for the Windows installer (below).

^^^^^^^^^^^^^^^^
Git
^^^^^^^^^^^^^^^^
Required to check out the repository.
On Windows, install Cygwin with Git support to checkout the repository
or install msysGit from http://code.google.com/p/msysgit/downloads/list.

^^^^^^^^^^^^^^^^
C++ Compiler
^^^^^^^^^^^^^^^^
The following compilers are known to work:

* GCC
* Clang
* MSVC

^^^^^^^^^^^^^^^^
CMake
^^^^^^^^^^^^^^^^
* **Purpose:** generates project build files
* **Minimum Version:** 2.6
* **Usage:** build time (required)
* **Download URL:** http://www.cmake.org/cmake/resources/software.html

^^^^^^^^^^^^^^^^
Boost
^^^^^^^^^^^^^^^^
* **Purpose:** C++ library
* **Minimum Version:** 1.36 (Linux), 1.40 (Windows)
* **Usage:** build time + runtime (required)
* **Download URL:** http://www.boost.org/users/download/
* **Download URL (Windows installer):** http://www.boostpro.com/download

^^^^^^^^^^^^^^^^
LibUSB
^^^^^^^^^^^^^^^^
* **Purpose:** USB-based hardware support
* **Minimum Version:** 1.0
* **Usage:** build time + runtime (optional)
* **Download URL:** http://sourceforge.net/projects/libusb/files/libusb-1.0/
* **Download URL (Windows binaries):** http://www.libusb.org/wiki/windows_backend#LatestBinarySnapshots

^^^^^^^^^^^^^^^^
Python
^^^^^^^^^^^^^^^^
* **Purpose:** used by Cheetah and utility scripts
* **Minimum Version:** 2.6
* **Usage:** build time + runtime utility scripts (required)
* **Download URL:** http://www.python.org/download/

^^^^^^^^^^^^^^^^
Cheetah
^^^^^^^^^^^^^^^^
* **Purpose:** source code generation
* **Minimum Version:** 2.0
* **Usage:** build time (required)
* **Download URL:** http://www.cheetahtemplate.org/download.html
* **Download URL (Windows installer):** http://feisley.com/python/cheetah/

**Alternative method:**
Install **setuptools**, and use the **easy_install** command to install Cheetah.
http://pypi.python.org/pypi/setuptools

^^^^^^^^^^^^^^^^
Doxygen
^^^^^^^^^^^^^^^^
* **Purpose:** generates HTML API documentation
* **Usage:** build time (optional)
* **Download URL:** http://www.stack.nl/~dimitri/doxygen/download.html#latestsrc

^^^^^^^^^^^^^^^^
Docutils
^^^^^^^^^^^^^^^^
* **Purpose:** generates HTML user manual
* **Usage:** build time (optional)
* **Download URL:** http://docutils.sourceforge.net/

**Alternate method:**
Install **setuptools**, and use the **easy_install** command to install Docutils.
http://pypi.python.org/pypi/setuptools

------------------------------------------------------------------------
Build Instructions (Unix)
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Generate Makefiles with CMake
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    cd <uhd-repo-path>/host
    mkdir build
    cd build
    cmake ../

Additionally, configuration variables can be passed into CMake via the command line.
The following common-use configuration variables are listed below:

* For a custom install prefix: **-DCMAKE_INSTALL_PREFIX=<install-path>**
* To install libs into lib64: **cmake -DLIB_SUFFIX=64**

Example usage:
::

    cmake -DCMAKE_INSTALL_PREFIX=/opt/uhd ../

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Build and install
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    make
    make test
    sudo make install

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Setup the library path (Linux)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Make sure that **libuhd.so** is in your **LD_LIBRARY_PATH**,
or add it to **/etc/ld.so.conf** and make sure to run:
::

    sudo ldconfig

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Setup the library path (Mac OS X)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Make sure that **libuhd.dylib** is in your **DYLD_LIBRARY_PATH**.

------------------------------------------------------------------------
Build Instructions (Windows)
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Generate the project with CMake
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Open the CMake GUI.
* Set the path to the source code: **<uhd-repo-path>/host**.
* Set the path to the build directory: **<uhd-repo-path>/host/build**.
* Make sure that the paths do not contain spaces.
* Click "Configure" and select "Microsoft Visual Studio 10".
* Set the build variables and click "Configure" again.
* Click "Generate", and a project file will be created in the build directory.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
LibUSB CMake notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On Windows, CMake does not have the advantage of **pkg-config**,
so we must manually tell CMake how to locate the LibUSB header and lib.

* From the CMake GUI, select "Advanced View".
* Set **LIBUSB_INCLUDE_DIRS** to the directory with **libusb.h**.
* Set **LIBUSB_LIBRARIES** to the full path for **libusb-1.0.lib**.

  * Recommend the static **libusb-1.0.lib** to simplify runtime dependencies.

* Check the box to enable USB support, click "Configure" and "Generate".

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Build the project in MSVC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Open the generated project file in MSVC.
* Change the build type from "Debug" to "Release".
* Select the "Build All" target, right-click, and choose "Build".
* Select the install target, right-click, and choose "Build".

**Note:** You may not have permission to build the install target.
You need to be an administrator or to run MSVC as administrator.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Build the project in MSVC (command line)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Open the Visual Studio Command Prompt Shorcut:
::

    cd <uhd-repo-path>\host\build
    DevEnv uhd.sln /build Release /project ALL_BUILD
    DevEnv uhd.sln /build Release /project INSTALL

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Setup the PATH environment variable
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Add the UHD bin path to **%PATH%** (usually **C:\\Program Files\\UHD\\bin**)

**Note:**
The default interface for editing environment variable paths in Windows is very poor.
We recommend using "Rapid Environment Editor" (http://www.rapidee.com) over the default editor.

------------------------------------------------------------------------
Post-Install Tasks
------------------------------------------------------------------------
For USB-based devices,
see the `USB Transport Application Notes <./transport.html#usb-transport-libusb>`_
for platform-specific post-installation tasks.
