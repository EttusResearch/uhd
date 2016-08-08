//
// Copyright 2016 Ettus Research LLC
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

#include <uhd/rfnoc/dma_fifo_block_ctrl.hpp>
#include "dma_fifo_core_3000.hpp"
#include "wb_iface_adapter.hpp"
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/types/wb_iface.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/mutex.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

//TODO (Ashish): This should come from the framework
static const double BUS_CLK_RATE = 166.67e6;

class dma_fifo_block_ctrl_impl : public dma_fifo_block_ctrl
{
public:
    static const uint32_t DEFAULT_SIZE = 32*1024*1024;

    UHD_RFNOC_BLOCK_CONSTRUCTOR(dma_fifo_block_ctrl)
    {
        _perifs.resize(get_input_ports().size());
        for(size_t i = 0; i < _perifs.size(); i++) {
            _perifs[i].ctrl = boost::make_shared<wb_iface_adapter>(
                // poke32 functor
                boost::bind(
                    static_cast< void (block_ctrl_base::*)(const boost::uint32_t, const boost::uint32_t, const size_t) >(&block_ctrl_base::sr_write),
                    this, _1, _2, i
                ),
                // peek32 functor
                boost::bind(
                    static_cast< boost::uint32_t (block_ctrl_base::*)(const boost::uint32_t, const size_t) >(&block_ctrl_base::user_reg_read32),
                    this,
                    _1, i
                ),
                // peek64 functor
                boost::bind(
                    static_cast< boost::uint64_t (block_ctrl_base::*)(const boost::uint32_t, const size_t) >(&block_ctrl_base::user_reg_read64),
                    this,
                    _1, i
                )
            );
            static const uint32_t USER_SR_BASE = 128*4;
            static const uint32_t USER_RB_BASE = 0;     //Don't care
            _perifs[i].base_addr = DEFAULT_SIZE*i;
            _perifs[i].depth = DEFAULT_SIZE;
            _perifs[i].core = dma_fifo_core_3000::make(_perifs[i].ctrl, USER_SR_BASE, USER_RB_BASE);
            _perifs[i].core->resize(_perifs[i].base_addr, _perifs[i].depth);
            UHD_MSG(status) << boost::format("[DMA FIFO] Running BIST for FIFO %d... ") % i;
            if (_perifs[i].core->ext_bist_supported()) {
                boost::uint32_t bisterr = _perifs[i].core->run_bist();
                if (bisterr != 0) {
                    throw uhd::runtime_error(str(boost::format("BIST failed! (code: %d)\n") % bisterr));
                } else {
                    double throughput = _perifs[i].core->get_bist_throughput(BUS_CLK_RATE);
                    UHD_MSG(status) << (boost::format("pass (Throughput: %.1fMB/s)") % (throughput/1e6)) << std::endl;
                }
            } else {
                if (_perifs[i].core->run_bist() == 0) {
                    UHD_MSG(status) << "pass\n";
                } else {
                    throw uhd::runtime_error("BIST failed!\n");
                }
            }
            _tree->access<int>(get_arg_path("base_addr/value", i))
                .add_coerced_subscriber(boost::bind(&dma_fifo_block_ctrl_impl::resize, this, _1, boost::ref(_perifs[i].depth), i))
                .set(_perifs[i].base_addr)
            ;
            _tree->access<int>(get_arg_path("depth/value", i))
                .add_coerced_subscriber(boost::bind(&dma_fifo_block_ctrl_impl::resize, this, boost::ref(_perifs[i].base_addr), _1, i))
                .set(_perifs[i].depth)
            ;
        }
    }

    void resize(const uint32_t base_addr, const uint32_t depth, const size_t chan) {
        boost::lock_guard<boost::mutex> lock(_config_mutex);
        _perifs[chan].base_addr = base_addr;
        _perifs[chan].depth = depth;
        _perifs[chan].core->resize(base_addr, depth);
    }

    uint32_t get_base_addr(const size_t chan) const {
        return _perifs[chan].base_addr;
    }

    uint32_t get_depth(const size_t chan) const {
        return _perifs[chan].depth;
    }

private:
    struct fifo_perifs_t
    {
        wb_iface::sptr           ctrl;
        dma_fifo_core_3000::sptr core;
        uint32_t                 base_addr;
        uint32_t                 depth;
    };
    std::vector<fifo_perifs_t> _perifs;

    boost::mutex _config_mutex;
};

UHD_RFNOC_BLOCK_REGISTER(dma_fifo_block_ctrl, "DmaFIFO");
