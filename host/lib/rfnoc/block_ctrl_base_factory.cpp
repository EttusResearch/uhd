//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/format.hpp>

#include <uhd/utils/log.hpp>
#include <uhd/rfnoc/blockdef.hpp>
#include <uhd/rfnoc/block_ctrl_base.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

typedef uhd::dict<std::string, block_ctrl_base::make_t> block_fcn_reg_t;
// Instantiate the block function registry container
UHD_SINGLETON_FCN(block_fcn_reg_t, get_block_fcn_regs);

void block_ctrl_base::register_block(
        const make_t &make,
        const std::string &key
) {
    if (get_block_fcn_regs().has_key(key)) {
        throw uhd::runtime_error(
            str(boost::format("Attempting to register an RFNoC block with key %s for the second time.") % key)
       );
    }

    get_block_fcn_regs().set(key, make);
}

/*! Look up names for blocks in XML files using NoC ID.
 */
static void lookup_block_key(uint64_t noc_id, make_args_t &make_args)
{
    try {
        blockdef::sptr bd = blockdef::make_from_noc_id(noc_id);
        if (not bd) {
            make_args.block_key  = DEFAULT_BLOCK_NAME;
            make_args.block_name = DEFAULT_BLOCK_NAME;
            return;
        }
        UHD_ASSERT_THROW(bd->is_block());
        make_args.block_key  = bd->get_key();
        make_args.block_name = bd->get_name();
        return;
    } catch (std::exception &e) {
        UHD_LOGGER_WARNING("RFNOC") << str(boost::format("Error while looking up name for NoC-ID %016X.\n%s") % noc_id % e.what()) ;
    }

    make_args.block_key  = DEFAULT_BLOCK_NAME;
    make_args.block_name = DEFAULT_BLOCK_NAME;
}


block_ctrl_base::sptr block_ctrl_base::make(
        const make_args_t &make_args_,
        uint64_t noc_id
) {
    UHD_LOGGER_TRACE("RFNOC") << "[RFNoC Factory] block_ctrl_base::make()";
    make_args_t make_args = make_args_;

    // Check if a block key was specified, in this case, we *must* either
    // create a specialized block controller class or throw
    if (make_args.block_key.empty()) {
        lookup_block_key(noc_id, make_args);
    } else if (not get_block_fcn_regs().has_key(make_args.block_key)) {
        throw uhd::runtime_error(
            str(boost::format("No block controller class registered for key '%s'.") % make_args.block_key)
        );
    }
    if (not get_block_fcn_regs().has_key(make_args.block_key)) {
        UHD_LOG_WARNING("RFNOC",
            "Can't find a block controller for key " << make_args.block_key
            << ", using default block controller!");
        make_args.block_key = DEFAULT_BLOCK_NAME;
    }
    if (make_args.block_name.empty()) {
        make_args.block_name = make_args.block_key;
    }

    UHD_LOGGER_TRACE("RFNOC")
        << "[RFNoC Factory] Using controller key '" << make_args.block_key
        << "' and block name '" << make_args.block_name << "'";
    return get_block_fcn_regs()[make_args.block_key](make_args);
}

