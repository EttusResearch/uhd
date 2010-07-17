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
C++
^^^^^^^^^^^^^^^^
On Unix, this is GCC 4.0 and above. On Windows, this is MSVC 2008.
Other compilers have not been tested yet or confirmed working.

^^^^^^^^^^^^^^^^
CMake
^^^^^^^^^^^^^^^^
* **Purpose:** generates project build files
* **Version:** at least 2.8
* **Required for:** build time
* **Download URL:** http://www.cmake.org/cmake/resources/software.html

^^^^^^^^^^^^^^^^
Boost
^^^^^^^^^^^^^^^^
* **Purpose:** C++ library
* **Version:** at least 3.6 unix, at least 4.0 windows
* **Required for:** build time + run time
* **Download URL:** http://www.boost.org/users/download/
* **Download URL (windows installer):** http://www.boostpro.com/download

^^^^^^^^^^^^^^^^
Python
^^^^^^^^^^^^^^^^
* **Purpose:** used by Cheetah and utility scripts
* **Version:** at least 2.6
* **Required for:** build time + run time utility scripts
* **Download URL:** http://www.python.org/download/

^^^^^^^^^^^^^^^^
Cheetah
^^^^^^^^^^^^^^^^
* **Purpose:** source code generation
* **Version:** at least 2.0
* **Required for:** build time
* **Download URL:** http://www.cheetahtemplate.org/download.html
* **Download URL (windows installer):** http://feisley.com/python/cheetah/

^^^^^^^^^^^^^^^^
Doxygen
^^^^^^^^^^^^^^^^
* **Purpose:** generates html api documentation
* **Required for:** build time (optional)
* **Download URL:** http://www.stack.nl/~dimitri/doxygen/download.html#latestsrc

^^^^^^^^^^^^^^^^
Docutils
^^^^^^^^^^^^^^^^
* **Purpose:** generates html user manual
* **Required for:** build time (optional)
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

**Notes:**

* For a custom prefix, use: cmake -DCMAKE_INSTALL_PREFIX=<prefix> ../

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
Build the project in MSVC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Open the generated project file in MSVC.
* Change the build type from "Debug" to "Release".
* Select the build all target, right click, and choose build.
* Select the install target, right click, and choose build.

**Note:** you may not have permission to build the install target.
You need to be an administrator or to run MSVC as administrator.

** alternative command line instructions **

* Open the Visual Studio Command Prompt Shorcut
* DevEnv <uhd-repo-path>\host\build\ALL_BUILD.vcproj /Build Release
* DevEnv <uhd-repo-path>\host\build\INSTALL.vcproj /Build Release

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Setup the PATH environment variable
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Add the boost library path to %PATH% (usually c:\\program files\\boost\\<version>\\lib)
* Add the uhd library path to %PATH% (usually c:\\program files\\uhd\\lib)
