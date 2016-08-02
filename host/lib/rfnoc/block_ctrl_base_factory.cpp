//
// Copyright 2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <boost/format.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/rfnoc/blockdef.hpp>
#include <uhd/rfnoc/block_ctrl_base.hpp>

#define UHD_FACTORY_LOG() UHD_LOGV(never)

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
static void lookup_block_key(boost::uint64_t noc_id, make_args_t &make_args)
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
        UHD_MSG(warning) << str(boost::format("Error while looking up name for NoC-ID %016X.\n%s") % noc_id % e.what()) << std::endl;
    }

    make_args.block_key  = DEFAULT_BLOCK_NAME;
    make_args.block_name = DEFAULT_BLOCK_NAME;
}


block_ctrl_base::sptr block_ctrl_base::make(
        const make_args_t &make_args_,
        boost::uint64_t noc_id
) {
    UHD_FACTORY_LOG() << "[RFNoC Factory] block_ctrl_base::make() " << std::endl;
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
        make_args.block_key = DEFAULT_BLOCK_NAME;
    }
    if (make_args.block_name.empty()) {
        make_args.block_name = make_args.block_key;
    }

    UHD_FACTORY_LOG() << "[RFNoC Factory] Using controller key '" << make_args.block_key << "' and block name '" << make_args.block_name << "'" << std::endl;
    return get_block_fcn_regs()[make_args.block_key](make_args);
}

