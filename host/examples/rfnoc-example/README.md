# RFNoC: An example out-of-tree module

This directory contains a fully functional out-of-tree module with a gain block.
It serves as an example for OOT modules with UHD 4.0 and above.

## Directory Structure

* `blocks`: This directory contains all the block definitions. These block
  definitions can be read by the RFNoC tools, and will get installed into the
  system for use by other out-of-tree modules.

* `cmake`: This directory only needs to be modified if this OOT module will
  come with its own custom CMake modules.

* `fpga`: This directory contains the source code for the HDL modules of the
  individual RFNoC blocks, along with their testbenches, and additional modules
  required to build the blocks. There is one subdirectory for every block.

* `include/rfnoc/example`: Here, all the header files for the block controllers
  are stored, along with any other include files that should be installed when
  installing this OOT module.

* `lib`: Here, all the non-header source files for the block controllers are stored,
  along with any other include file that should be installed when installing
  this OOT module. This includes the block controller cpp files.

* `apps`: This contains an example application that links against UHD and this
  OOT module. The app does not get installed, it resides in the build directory.
