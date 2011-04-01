========================================================================
UHD - Build Guide
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Build Dependencies
------------------------------------------------------------------------

**Linux Notes:**
The dependencies can be acquired through the package manager.

**Mac OS X Notes:**
Install the "Xcode Developer Tools" to get the build tools (gcc and make).
Use MacPorts to get the Boost and Cheetah dependencies.
Other dependencies can be downloaded as dmg installers from the web.

**Windows Notes:**
The dependencies can be acquired through installable exe files.
Usually, the windows installer can be found on the project's website.
Some projects do not host windows installers, and if this is the case,
follow the auxiliary download url for the windows installer (below).

^^^^^^^^^^^^^^^^
Git
^^^^^^^^^^^^^^^^
Required to check out the repository.
On windows, install cygwin with git support to checkout the repository,
or install msysgit from http://code.google.com/p/msysgit/downloads/list

^^^^^^^^^^^^^^^^
C++ compiler
^^^^^^^^^^^^^^^^
The following compilers are known to work:

* GCC
* Clang
* MSVC

^^^^^^^^^^^^^^^^
CMake
^^^^^^^^^^^^^^^^
* **Purpose:** generates project build files
* **Version:** at least 2.6
* **Usage:** build time (required)
* **Download URL:** http://www.cmake.org/cmake/resources/software.html

^^^^^^^^^^^^^^^^
Boost
^^^^^^^^^^^^^^^^
* **Purpose:** C++ library
* **Version:** at least 1.36 unix, at least 1.40 windows
* **Usage:** build time + run time (required)
* **Download URL:** http://www.boost.org/users/download/
* **Download URL (windows installer):** http://www.boostpro.com/download

^^^^^^^^^^^^^^^^
LibUSB
^^^^^^^^^^^^^^^^
* **Purpose:** USB-based hardware support
* **Version:** at least 1.0
* **Usage:** build time + run time (optional)
* **Download URL:** http://sourceforge.net/projects/libusb/files/libusb-1.0/
* **Download URL (windows binaries):** http://www.libusb.org/wiki/windows_backend#LatestBinarySnapshots

^^^^^^^^^^^^^^^^
Python
^^^^^^^^^^^^^^^^
* **Purpose:** used by Cheetah and utility scripts
* **Version:** at least 2.6
* **Usage:** build time + run time utility scripts (required)
* **Download URL:** http://www.python.org/download/

^^^^^^^^^^^^^^^^
Cheetah
^^^^^^^^^^^^^^^^
* **Purpose:** source code generation
* **Version:** at least 2.0
* **Usage:** build time (required)
* **Download URL:** http://www.cheetahtemplate.org/download.html
* **Download URL (windows installer):** http://feisley.com/python/cheetah/

**Alternative method:**
Install setuptools, and use the easy_install command to install Cheetah.
http://pypi.python.org/pypi/setuptools

^^^^^^^^^^^^^^^^
Doxygen
^^^^^^^^^^^^^^^^
* **Purpose:** generates html api documentation
* **Usage:** build time (optional)
* **Download URL:** http://www.stack.nl/~dimitri/doxygen/download.html#latestsrc

^^^^^^^^^^^^^^^^
Docutils
^^^^^^^^^^^^^^^^
* **Purpose:** generates html user manual
* **Usage:** build time (optional)
* **Download URL:** http://docutils.sourceforge.net/

------------------------------------------------------------------------
Build Instructions (Unix)
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Generate Makefiles with cmake
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    cd <uhd-repo-path>/host
    mkdir build
    cd build
    cmake ../

Additionally, configuration variables can be passed into cmake via the command line.
The following common-use configuration variables are listed below:

* For a custom install prefix: -DCMAKE_INSTALL_PREFIX=<prefix>
* To install libs into lib64: cmake -DLIB_SUFFIX=64

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
Make sure that libuhd.so is in your LD_LIBRARY_PATH
or add it to /etc/ld.so.conf and make sure to run sudo ldconfig

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Setup the library path (Mac OS X)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Make sure that libuhd.dylib is in your DYLD_LIBRARY_PATH

------------------------------------------------------------------------
Build Instructions (Windows)
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Generate the project with cmake
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Open the cmake gui program.
* Set the path to the source code: <uhd-repo-path>/host
* Set the path to the build directory: <uhd-repo-path>/host/build
* Make sure that the paths do not contain spaces.
* Click configure and select the MSVC compiler.
* Set the build variables and click configure again.
* Click generate and a project file will be created in the build directory.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
LibUSB cmake notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On Windows, cmake does not have the advantage of pkg-config,
so we must manually tell cmake how to locate the LibUSB header and lib.

* From the cmake gui, select "Advanded View"
* Set LIBUSB_INCLUDE_DIR to the directory with "libusb.h".
* Set LIBUSB_LIBRARIES to the full path for "libusb-1.0.lib".

  * Recommend the static libusb-1.0.lib to simplify runtime dependencies.

* Check the box to enable USB support, click configure and generate.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Build the project in MSVC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Open the generated project file in MSVC.
* Change the build type from "Debug" to "Release".
* Select the build all target, right click, and choose build.
* Select the install target, right click, and choose build.

**Note:** you may not have permission to build the install target.
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
* Add the uhd bin path to %PATH% (usually c:\\program files\\uhd\\bin)

**Note:**
The interface for editing environment variable paths in Windows is very poor.
I recommend using "Rapid Environment Editor" (http://www.rapidee.com) over the default editor.
