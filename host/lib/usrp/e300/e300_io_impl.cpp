//
// Copyright 2013-2014 Ettus Research LLC
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

#include "e300_regs.hpp"
#include "e300_impl.hpp"
#include "e300_fpga_defs.hpp"
#include "e300_defaults.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include <uhd/utils/tasks.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

namespace uhd { namespace usrp { namespace e300 {

uhd::device_addr_t e300_impl::get_rx_hints(size_t)
{
    return uhd::device_addr_t(str(boost::format("max_recv_window=%d") % DEFAULT_RX_DATA_NUM_FRAMES));
}

/***********************************************************************
 * Legacy Subdev-spec support
 **********************************************************************/
void e300_impl::subdev_to_blockid(
        const subdev_spec_pair_t &spec, const size_t mb_i,
        rfnoc::block_id_t &block_id, device_addr_t &args
) {
    UHD_ASSERT_THROW(spec.db_name == "A");
    UHD_ASSERT_THROW(spec.sd_name == "A" || spec.sd_name == "B");

    block_id.set_device_no(mb_i);
    block_id.set_block_name("Radio");
    block_id.set_block_count(0);
    args["block_port"] = spec.sd_name == "A" ? "0" : "1";
}

subdev_spec_pair_t e300_impl::blockid_to_subdev(
        const rfnoc::block_id_t &block_id, const uhd::device_addr_t &args
) {
    UHD_ASSERT_THROW(block_id.get_block_count() == 0);
    UHD_ASSERT_THROW(block_id.get_block_name() == "Radio");

    subdev_spec_pair_t spec;
    spec.db_name = "A";
    spec.sd_name = (args.cast<size_t>("block_port", 0) == 1 ? "B" : "A");
    return spec;
}

}}} // namespace
