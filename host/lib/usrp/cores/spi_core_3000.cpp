//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhdlib/usrp/cores/spi_core_3000.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

namespace {
constexpr double DEFAULT_DIVIDER = 30.0;
}

using namespace uhd;

spi_core_3000::~spi_core_3000(void)
{
    /* NOP */
}

class spi_core_3000_impl : public spi_core_3000
{
public:
    spi_core_3000_impl(poke32_fn_t&& poke32_fn,
        peek32_fn_t&& peek32_fn,
        const size_t base,
        const size_t reg_offset,
        const size_t readback)
        : _poke32(std::move(poke32_fn))
        , _peek32(std::move(peek32_fn))
        , _spi_div_addr(base + 0 * reg_offset)
        , _spi_ctrl_addr(base + 1 * reg_offset)
        , _spi_data_addr(base + 2 * reg_offset)
        , _readback_addr(readback)
    {
        this->set_divider(DEFAULT_DIVIDER);
    }

    uint32_t transact_spi(int which_slave,
        const spi_config_t& config,
        uint32_t data,
        size_t num_bits,
        bool readback) override
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // load SPI divider
        size_t spi_divider = _div;
        if (config.use_custom_divider) {
            // The resulting SPI frequency will be f_system/(2*(divider+1))
            // This math ensures the frequency will be equal to or less than the target
            spi_divider = (config.divider - 1) / 2;
        }

        // conditionally send SPI divider
        if (spi_divider != _divider_cache) {
            _poke32(_spi_div_addr, spi_divider);
            _divider_cache = spi_divider;
        }

        // load control word
        uint32_t ctrl_word = 0;
        ctrl_word |= ((which_slave & 0xffffff) << 0);
        ctrl_word |= ((num_bits & 0x3f) << 24);
        if (config.mosi_edge == spi_config_t::EDGE_FALL)
            ctrl_word |= (1 << 31);
        if (config.miso_edge == spi_config_t::EDGE_RISE)
            ctrl_word |= (1 << 30);

        // conditionally send control word
        if (_ctrl_word_cache != ctrl_word) {
            _poke32(_spi_ctrl_addr, ctrl_word);
            _ctrl_word_cache = ctrl_word;
        }

        // load data word (must be in upper bits)
        const uint32_t data_out = data << (32 - num_bits);

        // send data word
        _poke32(_spi_data_addr, data_out);

        // conditional readback
        if (readback) {
            return _peek32(_readback_addr);
        }

        return 0;
    }

    void set_divider(const double div) override
    {
        _div = size_t((div / 2) - 0.5);
    }

private:
    poke32_fn_t _poke32;
    peek32_fn_t _peek32;
    const size_t _spi_div_addr;
    const size_t _spi_ctrl_addr;
    const size_t _spi_data_addr;
    const size_t _readback_addr;
    uint32_t _ctrl_word_cache = 0;
    std::mutex _mutex;
    size_t _div;
    size_t _divider_cache = 0;
};

spi_core_3000::sptr spi_core_3000::make(
    wb_iface::sptr iface, const size_t base, const size_t readback)
{
    return std::make_shared<spi_core_3000_impl>(
        [iface](
            const uint32_t addr, const uint32_t value) { iface->poke32(addr, value); },
        [iface](const uint32_t addr) { return iface->peek32(addr); },
        base,
        4,
        readback);
}

spi_core_3000::sptr spi_core_3000::make(spi_core_3000::poke32_fn_t&& poke32_fn,
    spi_core_3000::peek32_fn_t&& peek32_fn,
    const size_t base,
    const size_t reg_offset,
    const size_t readback)
{
    return std::make_shared<spi_core_3000_impl>(
        std::move(poke32_fn), std::move(peek32_fn), base, reg_offset, readback);
}
