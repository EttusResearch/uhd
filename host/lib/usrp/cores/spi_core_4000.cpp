//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/features/spi_getter_iface.hpp>
#include <uhdlib/usrp/cores/spi_core_4000.hpp>
#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>


namespace {
constexpr uint32_t SPI_PERIPH_MOSI_EDGE_MASK = 1u << 28;
constexpr uint32_t SPI_PERIPH_MISO_EDGE_MASK = 1u << 27;
constexpr uint32_t SPI_PERIPH_NUM_BITS_MASK  = 0x7F;
constexpr uint32_t SPI_PERIPH_NUM_BITS_SHIFT = 20;
constexpr uint32_t SPI_PERIPH_PIN_MASK       = 0x1F;
constexpr uint32_t SPI_PERIPH_CS_SHIFT       = 15;
constexpr uint32_t SPI_PERIPH_SDI_SHIFT      = 10;
constexpr uint32_t SPI_PERIPH_SDO_SHIFT      = 5;
constexpr uint32_t SPI_PERIPH_CLK_SHIFT      = 0;

constexpr uint32_t SPI_TRANSACTION_PERIPH_MASK   = 0x3;
constexpr uint32_t SPI_TRANSACTION_PERIPH_SHIFT  = 16;
constexpr uint32_t SPI_TRANSACTION_DIVIDER_MASK  = 0xFFFF;
constexpr uint32_t SPI_TRANSACTION_DIVIDER_SHIFT = 0;

constexpr uint32_t SPI_STATUS_READY_MASK    = 1u << 31;
constexpr uint32_t SPI_STATUS_RESPONSE_MASK = 0x7FFFFFFF;
} // namespace

namespace uhd { namespace cores {

class spi_core_4000_impl : public spi_core_4000
{
public:
    spi_core_4000_impl(poke32_fn_t&& poke32_fn,
        poke64_fn_t&& poke64_fn,
        peek32_fn_t&& peek32_fn,
        poll32_fn_t&& poll32_fn,
        const size_t spi_periph_cfg,
        const size_t spi_transaction_cfg,
        const size_t spi_transaction_go,
        const size_t spi_transaction_lsb,
        const size_t spi_status,
        const size_t spi_full_response_msb,
        const size_t spi_controller_info)
        : _poke32(std::move(poke32_fn))
        , _poke64(std::move(poke64_fn))
        , _peek32(std::move(peek32_fn))
        , _poll32(std::move(poll32_fn))
        , _spi_periph_cfg(spi_periph_cfg)
        , _spi_transaction_cfg(spi_transaction_cfg)
        , _spi_transaction_go(spi_transaction_go)
        , _spi_transaction_lsb(spi_transaction_lsb)
        , _spi_status(spi_status)
        , _spi_full_response_msb(spi_full_response_msb)
        , _spi_ctrl_info(spi_controller_info)
    {
    }

    void set_spi_periph_config(
        const std::vector<uhd::features::spi_periph_config_t>& spc) override
    {
        const size_t num_periphs_allowed = _peek32(_spi_ctrl_info) & 0xF;
        if (spc.size() > num_periphs_allowed) {
            throw uhd::value_error("Number of configurations passed ("
                                   + std::to_string(spc.size())
                                   + ") exceeds the maximum number allowed by "
                                     "the bitfile per radio block. Maximum number: "
                                   + std::to_string(num_periphs_allowed));
        }

        _spi_periph_config = spc;
        _periph_ctrl_cache.assign(_spi_periph_config.size(), 0);
    }

    uint32_t transact_spi(const int which_periph,
        const spi_config_t& config,
        const uint32_t data,
        const size_t num_bits,
        const bool readback) override
    {
        return transact_spi64(which_periph, config, data, num_bits, readback)
               & 0xFFFFFFFF;
    }

    uint64_t transact_spi64(const int which_periph,
        const spi_config_t& config,
        const uint64_t data,
        const size_t num_bits,
        const bool readback) override
    {
        if (static_cast<uint32_t>(which_periph) >= _spi_periph_config.size()) {
            throw uhd::value_error(
                "No configuration given for requested SPI peripheral.");
        }
        size_t divider = 4;
        if (config.use_custom_divider) {
            divider = config.divider;
        }
        if (divider > 0xFFFF) {
            throw uhd::value_error("Clock divider exceeds maximum value (65535).");
        }
        if (num_bits > 64 || num_bits == 0) {
            throw uhd::value_error("Number of bits must be between 1 and 64.");
        }
        std::lock_guard<std::mutex> lock(_mutex);
        uint32_t periph_ctrl = 0;
        if (config.mosi_edge == spi_config_t::EDGE_FALL) {
            periph_ctrl |= SPI_PERIPH_MOSI_EDGE_MASK;
        }
        if (config.miso_edge == spi_config_t::EDGE_RISE) {
            periph_ctrl |= SPI_PERIPH_MISO_EDGE_MASK;
        }
        periph_ctrl |= (num_bits & SPI_PERIPH_NUM_BITS_MASK) << SPI_PERIPH_NUM_BITS_SHIFT;

        /* In the FPGA, the offset between Port A and B is 16, but we only have 12 pins
         * per port. To align this with the rest of the GPIO API, we need to adjust the
         * pin numbers accordingly.
         */
        const auto get_bitfield = [](uint32_t pin, uint32_t offset) -> uint32_t {
            const uint32_t adj = pin
                                 + ((pin >= NUM_PINS_PER_PORT) ? static_cast<uint32_t>(
                                        PORT_NUMBER_OFFSET - NUM_PINS_PER_PORT)
                                                               : 0u);
            return (adj & SPI_PERIPH_PIN_MASK) << offset;
        };
        // periph_cs (which GPIO line for CS signal)
        periph_ctrl |=
            get_bitfield(_spi_periph_config[which_periph].periph_cs, SPI_PERIPH_CS_SHIFT);
        // periph_sdi (which GPIO line for serial data in from peripheral)
        periph_ctrl |= get_bitfield(
            _spi_periph_config[which_periph].periph_sdi, SPI_PERIPH_SDI_SHIFT);
        // periph_sdo (which GPIO line for serial data out to peripheral)
        periph_ctrl |= get_bitfield(
            _spi_periph_config[which_periph].periph_sdo, SPI_PERIPH_SDO_SHIFT);
        // periph_clk (which GPIO line for clk signal)
        periph_ctrl |= get_bitfield(
            _spi_periph_config[which_periph].periph_clk, SPI_PERIPH_CLK_SHIFT);

        // conditionally send peripheral control
        if (_periph_ctrl_cache[which_periph] != periph_ctrl) {
            _poke32(_spi_periph_cfg + (which_periph * 0x4), periph_ctrl);
            _periph_ctrl_cache[which_periph] = periph_ctrl;
        }

        uint32_t transaction_config = 0;
        // SPI chip select
        transaction_config |= (which_periph & SPI_TRANSACTION_PERIPH_MASK)
                              << SPI_TRANSACTION_PERIPH_SHIFT;
        // SPI clock divider
        transaction_config |= (divider & SPI_TRANSACTION_DIVIDER_MASK)
                              << SPI_TRANSACTION_DIVIDER_SHIFT;

        // conditionally send transaction config
        if (_transaction_cfg_cache != transaction_config) {
            _poke32(_spi_transaction_cfg, transaction_config);
            _transaction_cfg_cache = transaction_config;
        }

        if (num_bits > 32) {
            _poke64(_spi_transaction_lsb, data << (64 - num_bits));
        } else {
            // load data word (in upper bits)
            const uint32_t data_out =
                static_cast<uint32_t>((data << (32 - num_bits)) & 0xFFFFFFFF);
            _poke32(_spi_transaction_go, data_out);
        }

        // conditional readback
        if (readback) {
            uint64_t response = 0;
            try {
                response = _poll32(_spi_status,
                               SPI_STATUS_READY_MASK,
                               SPI_STATUS_READY_MASK,
                               5.0,
                               true)
                           & SPI_STATUS_RESPONSE_MASK;
            } catch (uhd::op_failed&) {
                throw uhd::io_error(
                    "SPI Read did not receive a SPI Ready within 5 seconds");
                return 0;
            }

            if (num_bits > 31) {
                // Read and filter bits 32 to 63 of the response if applicable
                const size_t msb_bits   = std::min(num_bits - 31, size_t(32));
                const uint64_t msb_mask = (UINT64_C(1) << msb_bits) - 1;
                response |= (_peek32(_spi_full_response_msb) & msb_mask) << 31;
            } else {
                // Filter the response to only include the valid number of bits
                response &= (UINT64_C(1) << num_bits) - 1;
            }
            return response;
        }

        return 0;
    }

private:
    poke32_fn_t _poke32;
    poke64_fn_t _poke64;
    peek32_fn_t _peek32;
    poll32_fn_t _poll32;
    const size_t _spi_periph_cfg;
    const size_t _spi_transaction_cfg;
    const size_t _spi_transaction_go;
    const size_t _spi_transaction_lsb;
    const size_t _spi_status;
    const size_t _spi_full_response_msb;
    const size_t _spi_ctrl_info;
    std::vector<uint32_t> _periph_ctrl_cache;
    // Initialize with invalid value.
    uint32_t _transaction_cfg_cache = std::numeric_limits<uint32_t>::max();
    std::mutex _mutex;
    std::vector<uhd::features::spi_periph_config_t> _spi_periph_config;

    /*! Find out if we timed out */
    bool spi_timeout(std::chrono::steady_clock::time_point start, uint32_t timeout_s)
    {
        using namespace std::chrono;
        return (duration_cast<seconds>(steady_clock::now() - start)).count() > timeout_s;
    }
};

spi_core_4000::sptr spi_core_4000::make(spi_core_4000::poke32_fn_t&& poke32_fn,
    spi_core_4000::poke64_fn_t&& poke64_fn,
    spi_core_4000::peek32_fn_t&& peek32_fn,
    spi_core_4000::poll32_fn_t&& poll32_fn,
    const size_t spi_periph_cfg,
    const size_t spi_transaction_cfg,
    const size_t spi_transaction_go,
    const size_t spi_transaction_lsb,
    const size_t spi_status,
    const size_t spi_full_response_msb,
    const size_t spi_controller_info)
{
    return std::make_shared<spi_core_4000_impl>(std::move(poke32_fn),
        std::move(poke64_fn),
        std::move(peek32_fn),
        std::move(poll32_fn),
        spi_periph_cfg,
        spi_transaction_cfg,
        spi_transaction_go,
        spi_transaction_lsb,
        spi_status,
        spi_full_response_msb,
        spi_controller_info);
}

}} // namespace uhd::cores
