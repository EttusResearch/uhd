//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/features/spi_getter_iface.hpp>
#include <uhdlib/usrp/cores/gpio_port_mapper.hpp>
#include <uhdlib/usrp/cores/spi_core_4000.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>


namespace uhd { namespace cores {

class spi_core_4000_impl : public spi_core_4000
{
public:
    spi_core_4000_impl(poke32_fn_t&& poke32_fn,
        peek32_fn_t&& peek32_fn,
        const size_t spi_slave_cfg,
        const size_t spi_transaction_cfg,
        const size_t spi_transaction_go,
        const size_t spi_status,
        const size_t spi_controller_info,
        const mapper_sptr port_mapper)
        : _poke32(std::move(poke32_fn))
        , _peek32(std::move(peek32_fn))
        , _spi_slave_cfg(spi_slave_cfg)
        , _spi_transaction_cfg(spi_transaction_cfg)
        , _spi_transaction_go(spi_transaction_go)
        , _spi_status(spi_status)
        , _spi_ctrl_info(spi_controller_info)
        , _port_mapper(port_mapper)
    {
    }

    void set_spi_slave_config(
        const std::vector<uhd::features::spi_slave_config_t>& ssc) override
    {
        const size_t num_slaves_allowed = _peek32(_spi_ctrl_info) & 0xF;
        if (ssc.size() > num_slaves_allowed) {
            throw uhd::value_error("Number of configurations passed ("
                                   + std::to_string(ssc.size())
                                   + ") exceeds the maximum number allowed by "
                                     "the bitfile per radio block. Maximum number: "
                                   + std::to_string(num_slaves_allowed));
        }

        _spi_slave_config = ssc;
        _slave_ctrl_cache.assign(_spi_slave_config.size(), 0);
    }

    uint32_t transact_spi(const int which_slave,
        const spi_config_t& config,
        const uint32_t data,
        const size_t num_bits,
        const bool readback) override
    {
        if (static_cast<uint32_t>(which_slave) >= _spi_slave_config.size()) {
            throw uhd::value_error("No configuration given for requested SPI slave.");
        }
        if (config.divider > 0xFFFF) {
            throw uhd::value_error("Clock divider exceeds maximum value (65535).");
        }
        std::lock_guard<std::mutex> lock(_mutex);
        uint32_t slave_ctrl = 0;
        if (config.mosi_edge == spi_config_t::EDGE_FALL) {
            slave_ctrl |= (1 << 27);
        }
        if (config.miso_edge == spi_config_t::EDGE_RISE) {
            slave_ctrl |= (1 << 26);
        }
        slave_ctrl |= ((num_bits & 0x3F) << 20);
        // slave_ss (which GPIO line for CS signal)
        slave_ctrl |=
            _port_mapper->map_value(_spi_slave_config[which_slave].slave_ss & 0x1F) << 15;
        // slave_miso (which GPIO line for MISO signal)
        slave_ctrl |=
            _port_mapper->map_value(_spi_slave_config[which_slave].slave_miso & 0x1F)
            << 10;
        // slave_mosi (which GPIO line for MOSI signal)
        slave_ctrl |=
            _port_mapper->map_value(_spi_slave_config[which_slave].slave_mosi & 0x1F)
            << 5;
        // slave_clk (which GPIO line for clk signal)
        slave_ctrl |=
            _port_mapper->map_value(_spi_slave_config[which_slave].slave_clk & 0x1F) << 0;

        // conditionally send slave control
        if (_slave_ctrl_cache[which_slave] != slave_ctrl) {
            _poke32(_spi_slave_cfg + (which_slave * 0x4), slave_ctrl);
            _slave_ctrl_cache[which_slave] = slave_ctrl;
        }

        uint32_t transaction_config = 0;
        // SPI slave select
        transaction_config |= ((which_slave & 0x3) << 16);
        // SPI clock divider
        transaction_config |= ((config.divider & 0xFFFF) << 0);

        // conditionally send transaction config
        if (_transaction_cfg_cache != transaction_config) {
            _poke32(_spi_transaction_cfg, transaction_config);
            _transaction_cfg_cache = transaction_config;
        }

        // load data word (in upper bits)
        const uint32_t data_out = data << (32 - num_bits);

        // send data word
        _poke32(_spi_transaction_go, data_out);

        // conditional readback
        if (readback) {
            uint32_t spi_response = 0;
            bool spi_ready        = false;
            // Poll the SPI status until we get a SPI Ready flag
            std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
            while (!spi_ready) {
                spi_response = _peek32(_spi_status);
                spi_ready    = spi_ready_bit(spi_response);
                if (spi_timeout(t1, 5)) {
                    throw uhd::io_error(
                        "SPI Read did not receive a SPI Ready within 5 seconds");
                    return 0;
                }
            }
            return (0xFFFFFF & spi_response);
        }

        return 0;
    }

private:
    poke32_fn_t _poke32;
    peek32_fn_t _peek32;
    const size_t _spi_slave_cfg;
    const size_t _spi_transaction_cfg;
    const size_t _spi_transaction_go;
    const size_t _spi_status;
    const size_t _spi_ctrl_info;
    const mapper_sptr _port_mapper;
    std::vector<uint32_t> _slave_ctrl_cache;
    uint32_t _transaction_cfg_cache = 0;
    std::mutex _mutex;
    std::vector<uhd::features::spi_slave_config_t> _spi_slave_config;

    /*! Gets the SPI_READY flag */
    bool spi_ready_bit(uint32_t spi_response)
    {
        return (spi_response >> 24) & 0x1;
    }

    /*! Find out if we timed out */
    bool spi_timeout(std::chrono::steady_clock::time_point start, uint32_t timeout_s)
    {
        using namespace std::chrono;
        return (duration_cast<seconds>(steady_clock::now() - start)).count() > timeout_s;
    }
};

spi_core_4000::sptr spi_core_4000::make(spi_core_4000::poke32_fn_t&& poke32_fn,
    spi_core_4000::peek32_fn_t&& peek32_fn,
    const size_t spi_slave_cfg,
    const size_t spi_transaction_cfg,
    const size_t spi_transaction_go,
    const size_t spi_status,
    const size_t spi_controller_info,
    const mapper_sptr port_mapper)
{
    return std::make_shared<spi_core_4000_impl>(std::move(poke32_fn),
        std::move(peek32_fn),
        spi_slave_cfg,
        spi_transaction_cfg,
        spi_transaction_go,
        spi_status,
        spi_controller_info,
        port_mapper);
}

}} // namespace uhd::cores
