# RFNoC: An example out-of-tree module containing a gain block

This directory contains a fully functional out-of-tree module with a gain block.
It serves as an example for OOT modules with UHD 4.7 and above.

We recommend sticking to this directory structure and file layout for other
out-of-tree blocks.

## Directory Structure (in alphabetical order)

* `apps`: This directory is for applications that should get installed into
  the $PATH.

* `cmake`: This directory only needs to be modified if this OOT module will
  come with its own custom CMake modules.

* `examples`: Any example that shows how to use this OOT module goes here.
  Examples that are listed in CMake get installed into `share/rfnoc-gain/examples`
  within the installation prefix.

* `fpga/gain`: This directory contains the gateware for the HDL modules
  of the individual RFNoC blocks, along with their testbenches, and additional
  modules required to build the blocks. There is one subdirectory for every
  block (or module, or transport adapter). This is also where to define IP that
  is required to build the modules (e.g., Vivado-generated IP).
  Note that this is an "include directory" for gateware, which is why the
  module name ('gain') is included in the path. When installing this OOT
  module, these files get copied to the installation path's RFNoC package dir
  (e.g., `/usr/share/uhd/rfnoc/fpga/gain`) so the image builder can find
  all the source files when compiling bitfiles that use multiple OOT modules.

* `gr-rfnoc_gain`: An out-of-tree module for GNU Radio, which depends on this
  RFNoC out-of-tree module and provides GNU Radio and GNU Radio Companion
  bindings for using the gain block from within GNU Radio.

* `icores`: Stores full image core files. YAML files in this directory get
  picked up by CMake and get turned into build targets. For example, here we
  include an image core file called `x310_rfnoc_image_core.yml` which defines
  an X310 image core with the gain block included. You can build this image
  directly using the image builder, but you can also run `make x310_rfnoc_image_core`
  to build it using `make`.
  These files do not get installed.

* `include/rfnoc/gain`: Here, all the header files for the block controllers
  are stored, along with any other include files that should be installed when
  installing this OOT module.
  As with the gateware, the path name mirrors the path after the installation,
  e.g., these header files could get installed to `/usr/include/rfnoc/gain`.
  By mirroring the path name, we can write
  `#include <rfnoc/gain/gain_block_control.hpp>` in our C++ source code, and
  the compilation will work for building this OOT module directly, or in 3rd
  party applications.

* `lib`: Here, all the non-header source files for the block controllers are stored,
  along with any other include file that should not be installed when installing
  this OOT module. This includes the block controller cpp files. All of these
  source files comprise the DLL that gets built from this OOT module.

* `python`: Use this directory to add Python bindings for blocks. Note that if
  the UHD Python API is not found (or Python dependencies for building bindings,
  such as pybind11, are not found) no Python bindings will be generated.

* `rfnoc`: This directory contains all the block definitions (YAML files).
  These block definitions can be read by the RFNoC tools, and will get
  installed into the system for use by other out-of-tree modules.
  Within this directory, there needs to be one subdirectory for every type of
  RFNoC component that this out-of-tree module contains: `blocks` for regular
  RFNoC blocks, `modules` for generic Verilog modules, and `transport_adapters`
  for RFNoC transport adapters.
  Like with the gateware, these get installed into the RFNoC package data
  directory, e.g., `/usr/share/uhd/rfnoc/blocks/*.yml`.

