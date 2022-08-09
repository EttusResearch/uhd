//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/dmafifo_block_control.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhdlib/usrp/cores/dma_fifo_core_3000.hpp>
#include <boost/format.hpp>
#include <mutex>

using namespace uhd;
using namespace uhd::rfnoc;

namespace {

constexpr uint64_t BYTES_PER_BIST = 8000000;
constexpr double BIST_TIMEOUT     = 0.5; // s
//! Address space between FIFO controls
const uint32_t REG_OFFSET = 128;

} // namespace

class dmafifo_block_control_impl : public dmafifo_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(dmafifo_block_control)
    {
        UHD_ASSERT_THROW(get_num_input_ports() == get_num_output_ports());
        set_action_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
        set_prop_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
        // This line is not strictly necessary, as ONE_TO_ONE is the default.
        // We set it make it explicit how this block works.
        set_mtu_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
        // Now init DMA/DRAM control
        _fifo_cores.reserve(get_num_input_ports());
        for (size_t i = 0; i < get_num_input_ports(); i++) {
            _fifo_cores.push_back(dma_fifo_core_3000::make(
                [this, i](const uint32_t addr, const uint32_t data) {
                    regs().poke32(addr + i * REG_OFFSET, data);
                },
                [this, i](
                    const uint32_t addr) { return regs().peek32(addr + i * REG_OFFSET); },
                i));
            RFNOC_LOG_DEBUG("Initialized FIFO core " << i << ".");
            if (_fifo_cores.back()->has_bist()) {
                RFNOC_LOG_DEBUG("Running BIST...");
                const double throughput =
                    _fifo_cores.back()->run_bist(BYTES_PER_BIST, BIST_TIMEOUT);
                RFNOC_LOG_INFO(
                    boost::format("BIST passed (Estimated Minimum Throughput: %.0f MB/s)")
                    % (throughput / 1e6));
            } else {
                RFNOC_LOG_DEBUG("Channel " << i << " does not support BIST, skipping.");
            }
        }
    }

    uint32_t get_packet_count(const size_t chan)
    {
        UHD_ASSERT_THROW(chan < _fifo_cores.size());
        return _fifo_cores.at(chan)->get_packet_count();
    }

private:
    //! One FIFO core object per FIFO
    std::vector<dma_fifo_core_3000::sptr> _fifo_cores;
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    dmafifo_block_control, 0xF1F00000, "DmaFIFO", CLOCK_KEY_GRAPH, "bus_clk")
