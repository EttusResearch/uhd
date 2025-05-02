//
// Copyright ${year} ${copyright_holder}
//
// ${license}
//

#pragma once

#include <rfnoc/${MODULE_NAME}/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <cstdint>

namespace rfnoc { namespace ${MODULE_NAME} {

/*! Block controller: Describe me!
 */
class RFNOC_${MODULE_NAME.upper()}_API ${blockname}_block_control : public uhd::rfnoc::noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(${blockname}_block_control)

    // List all registers here if you need to know their address in the block controller:
    ////! The register address of the gain value
    //static const uint32_t REG_NAME;

};

}} // namespace rfnoc::gain
