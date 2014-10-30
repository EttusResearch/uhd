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
#include <uhd/usrp/rfnoc/blockdef.hpp>
#include <uhd/usrp/rfnoc/block_ctrl_base.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

typedef uhd::dict<std::string, block_ctrl_base::make_t> block_fcn_reg_t;
// Instantiate the block function registry container
UHD_SINGLETON_FCN(block_fcn_reg_t, get_block_fcn_regs);

void block_ctrl_base::register_block(
        const make_t &make,
        const std::string &name
) {
    if (get_block_fcn_regs().has_key(name)) {
        throw uhd::runtime_error(
            str(boost::format("Attempting to register an RFNoC block with name %s for the second time.") % name)
       );
    }

    get_block_fcn_regs().set(name, make);
}

/*! Look up names for blocks in XML files using NoC ID.
 */
static std::string lookup_block_name(boost::uint64_t noc_id)
{
    try {
        blockdef::sptr bd = blockdef::make_from_noc_id(noc_id);
        if (not bd) {
            return DEFAULT_BLOCK_NAME;
        }
        UHD_ASSERT_THROW(bd->is_block());
        UHD_MSG(status) << "created blockdef with name " << bd->get_name() << std::endl;
        return bd->get_name();
    } catch (std::exception &e) {
        UHD_MSG(warning) << str(boost::format("Error while looking up name for NoC-ID %016X.\n%s") % noc_id % e.what()) << std::endl;
    }

    return DEFAULT_BLOCK_NAME;
}


block_ctrl_base::sptr block_ctrl_base::make(
        const make_args_t &make_args,
        boost::uint64_t noc_id
) {
    UHD_MSG(status) << "[RFNoC Factory] block_ctrl_base::make() " << std::endl;
    make_args_t new_make_args = make_args;
    std::string key = make_args.block_name;

    // Check if a block name was provided, in this case, we *must* either
    // create an specialized block controller class or throw
    if (key.empty()) {
        key = new_make_args.block_name = lookup_block_name(noc_id);
    } else if (not get_block_fcn_regs().has_key(key)) {
        throw uhd::runtime_error(
            str(boost::format("No block controller class registered for type '%s'.") % key)
        );
    }
    UHD_MSG(status) << "[RFNoC Factory] Using controller key '" << key << "' and block name '" << new_make_args.block_name << "'" << std::endl;

    if (not get_block_fcn_regs().has_key(key)) {
        key = DEFAULT_BLOCK_NAME;
    }
    return get_block_fcn_regs()[key](new_make_args);
}

