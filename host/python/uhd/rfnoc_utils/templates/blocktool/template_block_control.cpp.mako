//
// Copyright ${year} ${copyright_holder}
//
// ${license}
//

// Include our own header:
#include <rfnoc/${MODULE_NAME}/${blockname}_block_control.hpp>

// These two includes are the minimum required to implement a block:
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/registry.hpp>

using namespace rfnoc::${MODULE_NAME};
using namespace uhd::rfnoc;

// Define register addresses here:
//const uint32_t ${blockname}_block_control::REG_NAME = 0x1234;

class ${blockname}_block_control_impl : public ${blockname}_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(${blockname}_block_control) {}


private:
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    ${blockname}_block_control, ${config["noc_id"]}, "${config["module_name"].capitalize()}", CLOCK_KEY_GRAPH, "bus_clk");
