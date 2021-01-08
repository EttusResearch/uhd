//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/cores/dma_fifo_core_3000.hpp>
#include <chrono>
#include <mutex>
#include <thread>

using namespace uhd;
using namespace std::chrono_literals;

namespace {

//! Info register: [0]: Indicates presence of BIST [31:16]: Magic word
const uint32_t REG_FIFO_INFO = 0x0000;
const uint16_t FIFO_MAGIC    = 0xF1F0;

//! Mem size register: [RAM Word Size | Address size]
const uint32_t REG_FIFO_MEM_SIZE      = 0x0008;
const uint32_t REG_FIFO_TIMEOUT       = 0x000C;
const uint32_t REG_FIFO_FULLNESS      = 0x0010;
const uint32_t REG_FIFO_ADDR_BASE     = 0x0018;
const uint32_t REG_FIFO_ADDR_MASK     = 0x0020;
const uint32_t REG_FIFO_PACKET_CNT    = 0x0028;
const uint32_t REG_BIST_CTRL          = 0x0030;
const uint32_t REG_BIST_CLK_RATE      = 0x0034;
const uint32_t REG_BIST_NUM_BYTES     = 0x0038;
const uint32_t REG_BIST_TX_BYTE_COUNT = 0x0040;
const uint32_t REG_BIST_RX_BYTE_COUNT = 0x0048;
const uint32_t REG_BIST_ERROR_COUNT   = 0x0050;
const uint32_t REG_BIST_CYCLE_COUNT   = 0x0058;

//! Read FIFO info register, check for magic, return "has bist" flag all in one
// go
bool unpack_fifo_info(const uint32_t fifo_info)
{
    if (fifo_info >> 16 != FIFO_MAGIC) {
        throw uhd::runtime_error("DMA FIFO: Incorrect magic value returned!");
    }

    return bool(fifo_info & 0x1);
}

} // namespace

dma_fifo_core_3000::~dma_fifo_core_3000(void)
{
    /* NOP */
}

class dma_fifo_core_3000_impl : public dma_fifo_core_3000
{
public:
    /**************************************************************************
     * Structors
     *************************************************************************/
    dma_fifo_core_3000_impl(
        poke32_fn_t&& poke_fn, peek32_fn_t&& peek_fn, const size_t fifo_index)
        : _fifo_index(fifo_index)
        , poke32(std::move(poke_fn))
        , peek32(std::move(peek_fn))
        , _has_bist(unpack_fifo_info(peek32(REG_FIFO_INFO)))
        , _mem_size(peek32(REG_FIFO_MEM_SIZE))
        , _bist_clk_rate(_has_bist ? peek32(REG_BIST_CLK_RATE) : 0.0)
    {
        UHD_LOG_DEBUG("DMA FIFO",
            "Initializing FIFO core "
                << _fifo_index << ": RAM Word Width: " << _mem_size.word_size << " bits, "
                << "address width: " << _mem_size.addr_size
                << " bits, base address: " << this->get_addr_base()
                << ", FIFO size: " << (uint64_t(get_addr_mask()) + 1) / 1024 / 1024
                << " MiB, has BIST: " << (_has_bist ? "Yes" : "No")
                << ", BIST clock rate: " << (_bist_clk_rate / 1e6)
                << " MHz, Initial FIFO fullness: "
                << dma_fifo_core_3000_impl::get_fifo_fullness() << ", FIFO timeout: "
                << dma_fifo_core_3000_impl::get_fifo_timeout() << " cycles");
    }

    ~dma_fifo_core_3000_impl() override {}

    /**************************************************************************
     * API
     *************************************************************************/
    bool has_bist() const override
    {
        return _has_bist;
    }

    uint64_t get_addr_size() const
    {
        return _mem_size.addr_size;
    }

    // TODO: read suppress API

    // fullness in bytes
    uint64_t get_fifo_fullness() override
    {
        return peek64(REG_FIFO_FULLNESS);
    }

    uint16_t get_fifo_timeout() override
    {
        return peek32(REG_FIFO_TIMEOUT) & 0xFFF;
    }

    void set_fifo_timeout(const uint16_t timeout_cycles) override
    {
        UHD_ASSERT_THROW(timeout_cycles <= 0xFFF);
        poke32(timeout_cycles, REG_FIFO_TIMEOUT);
    }

    uint64_t get_addr_base()
    {
        return peek64(REG_FIFO_ADDR_BASE);
    }

    uint64_t get_addr_mask()
    {
        return peek64(REG_FIFO_ADDR_MASK);
    }

    void set_addr_mask(uint64_t addr_mask)
    {
        return poke64(REG_FIFO_ADDR_MASK, addr_mask);
    }

    void set_addr_base(const uint64_t base_addr)
    {
        // FIXME verify we're within bounds
        poke64(REG_FIFO_ADDR_BASE, base_addr);
    }

    uint32_t get_packet_count() override
    {
        return peek32(REG_FIFO_PACKET_CNT);
    }

    /***** BIST controls *****************************************************/
    bool get_bist_running()
    {
        return bool(peek32(REG_BIST_CTRL) & (1 << 4));
    }

    void bist_clear_counters()
    {
        // strobe
        poke32(REG_BIST_CTRL, 1 << 2);
    }

    uint64_t get_bist_num_bytes()
    {
        return peek64(REG_BIST_NUM_BYTES);
    }

    void set_bist_num_bytes(uint64_t num_bytes)
    {
        poke64(REG_BIST_NUM_BYTES, num_bytes);
    }

    uint64_t get_bist_tx_byte_count()
    {
        return peek64(REG_BIST_TX_BYTE_COUNT);
    }

    uint64_t get_bist_rx_byte_count()
    {
        return peek64(REG_BIST_RX_BYTE_COUNT);
    }

    uint64_t get_bist_error_count()
    {
        return peek64(REG_BIST_ERROR_COUNT);
    }

    uint64_t get_bist_cycle_count()
    {
        return peek64(REG_BIST_CYCLE_COUNT);
    }

    void start_bist(const bool cont)
    {
        // strobe
        poke32(REG_BIST_CTRL, 1 | (cont ? (1 << 3) : 0));
    }

    void stop_bist()
    {
        // strobe
        poke32(REG_BIST_CTRL, 1 << 2);
    }

    double run_bist(const uint64_t num_bytes, const double timeout_s) override
    {
        // The number of cycles it will take to transfer all the BIST data if
        // there is a transfer on every clock cycle (this is the minimum time it
        // will take to execute the BIST):
        const uint64_t min_cycles = (num_bytes / (_mem_size.word_size / 8))
                                    + (num_bytes % (_mem_size.word_size / 8) ? 1 : 0);
        const auto min_duration =
            std::chrono::milliseconds(int64_t(min_cycles / _bist_clk_rate * 1e3));
        // The user could specify inconsistent values where the timeout is
        // smaller than the time it actually takes to execute the BIST, but we
        // simply interpret that as "as soon as possible" and don't throw an
        // error.
        const auto end_time = std::chrono::steady_clock::now()
                              + std::chrono::milliseconds(int64_t(timeout_s * 1000));
        bist_clear_counters();
        set_bist_num_bytes(num_bytes);
        // Start BIST in non-continuous mode
        start_bist(false);
        // Now wait at least for the minimum time it takes to execute the BIST
        std::this_thread::sleep_for(min_duration);
        // Now poll for finished until timeout
        while (get_bist_running()) {
            if (std::chrono::steady_clock::now() > end_time) {
                UHD_LOG_ERROR("DMA FIFO", "Timeout during BIST!");
                UHD_LOG_DEBUG("DMA FIFO",
                    "TX bytes transferred: "
                        << get_bist_tx_byte_count()
                        << " RX bytes transferred: " << get_bist_rx_byte_count()
                        << " Cycle count: " << get_bist_cycle_count());
                throw uhd::runtime_error("[DMA FIFO] Timeout during BIST!");
            }
            std::this_thread::sleep_for(5ms);
        }
        // Now the BIST is complete, we check all the transfers were accurate
        // and there were no errors
        bool error                = false;
        const uint64_t tx_bytes   = get_bist_tx_byte_count();
        const uint64_t rx_bytes   = get_bist_rx_byte_count();
        const uint64_t num_errors = get_bist_error_count();
        const uint64_t num_cycles = get_bist_cycle_count();
        const double bist_time    = double(num_cycles) / _bist_clk_rate;
        if (tx_bytes != num_bytes) {
            UHD_LOG_ERROR("DMA FIFO",
                "BIST Error: Incorrect number of TX bytes! "
                "Transmitted: "
                    << tx_bytes << " Expected: " << num_bytes);
            error = true;
        }
        if (rx_bytes != num_bytes) {
            UHD_LOG_ERROR("DMA FIFO",
                "BIST Error: Incorrect number of RX bytes! "
                "Received: "
                    << rx_bytes << " Expected: " << num_bytes);
            error = true;
        }
        if (num_errors != 0) {
            UHD_LOG_ERROR("DMA FIFO", "BIST Error: Error count is " << num_errors);
            error = true;
        }
        UHD_LOG_DEBUG("DMA FIFO",
            "BIST: Cycles elapsed: " << num_cycles << " Best Case: " << min_cycles);
        if (error) {
            throw uhd::runtime_error("[DMA FIFO] Bist failed!");
        }
        bist_clear_counters();

        // Return throughput in byte/s
        return num_bytes / bist_time;
    }

private:
    const size_t _fifo_index;
    poke32_fn_t poke32;
    peek32_fn_t peek32;

    //! Convenience function to do two consecutive peek32's
    uint64_t peek64(const uint32_t addr)
    {
        const uint32_t lo = peek32(addr);
        const uint32_t hi = peek32(addr + 4);
        return static_cast<uint64_t>(lo) | (static_cast<uint64_t>(hi) << 32);
    }

    //! Convenience function to do two consecutive poke32's
    void poke64(const uint32_t addr, const uint64_t data)
    {
        const uint32_t lo = static_cast<uint32_t>(data & 0xFFFFFFFF);
        const uint32_t hi = static_cast<uint32_t>((data >> 32) & 0xFFFFFFFF);
        poke32(addr, lo);
        poke32(addr + 4, hi);
    }

    //! True if we can do BISTs with this FIFO
    const bool _has_bist;
    struct mem_size_t
    {
        mem_size_t(const uint32_t mem_size)
            : word_size((mem_size >> 16) & 0xFFFF), addr_size(mem_size & 0xFFFF)
        {
        }

        //! RAM word width (e.g., 64)
        const uint16_t word_size;
        //! Width of the max available address size (in bits)
        const uint16_t addr_size;
    };
    //! Cache the contents of the MEM_SIZE register
    const mem_size_t _mem_size;

    //! Clock rate of the BIST circuitry in Hz
    const double _bist_clk_rate;
};

//
// Factory
//
dma_fifo_core_3000::sptr dma_fifo_core_3000::make(
    poke32_fn_t&& poke_fn, peek32_fn_t&& peek_fn, const size_t fifo_index)
{
    return std::make_shared<dma_fifo_core_3000_impl>(
        std::move(poke_fn), std::move(peek_fn), fifo_index);
}
