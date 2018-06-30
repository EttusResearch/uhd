//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/cores/spi_core_3000.hpp>
#include <boost/thread/thread.hpp> //sleep

#define SPI_DIV      _base + 0
#define SPI_CTRL     _base + 4
#define SPI_DATA     _base + 8
#define SPI_SHUTDOWN _base + 12

using namespace uhd;

spi_core_3000::~spi_core_3000(void){
    /* NOP */
}

class spi_core_3000_impl : public spi_core_3000
{
public:
    spi_core_3000_impl(wb_iface::sptr iface, const size_t base, const size_t readback):
        _iface(iface), _base(base), _readback(readback), _ctrl_word_cache(0), _divider_cache(0)
    {
        this->set_divider(30);
    }

    uint32_t transact_spi(
        int which_slave,
        const spi_config_t &config,
        uint32_t data,
        size_t num_bits,
        bool readback
    ){
        boost::lock_guard<boost::mutex> lock(_mutex);

        //load SPI divider
        size_t spi_divider = _div;
        if (config.use_custom_divider) {
            //The resulting SPI frequency will be f_system/(2*(divider+1))
            //This math ensures the frequency will be equal to or less than the target
            spi_divider = (config.divider-1)/2;
        }

        //conditionally send SPI divider
        if (spi_divider != _divider_cache) {
            _iface->poke32(SPI_DIV, spi_divider);
            _divider_cache = spi_divider;
        }

        //load control word
        uint32_t ctrl_word = 0;
        ctrl_word |= ((which_slave & 0xffffff) << 0);
        ctrl_word |= ((num_bits & 0x3f) << 24);
        if (config.mosi_edge == spi_config_t::EDGE_FALL) ctrl_word |= (1 << 31);
        if (config.miso_edge == spi_config_t::EDGE_RISE) ctrl_word |= (1 << 30);

        //conditionally send control word
        if (_ctrl_word_cache != ctrl_word)
        {
            _iface->poke32(SPI_CTRL, ctrl_word);
            _ctrl_word_cache = ctrl_word;
        }

        //load data word (must be in upper bits)
        const uint32_t data_out = data << (32 - num_bits);

        //send data word
        _iface->poke32(SPI_DATA, data_out);

        //conditional readback
        if (readback)
        {
            return _iface->peek32(_readback);
        }

        return 0;
    }

    void set_shutdown(const bool shutdown)
    {
        _shutdown_cache = shutdown;
        _iface->poke32(SPI_SHUTDOWN, _shutdown_cache);
    }

    bool get_shutdown()
    {
        return(_shutdown_cache);
    }

    void set_divider(const double div)
    {
        _div = size_t((div/2) - 0.5);
    }

private:

    wb_iface::sptr _iface;
    const size_t _base;
    const size_t _readback;
    uint32_t _ctrl_word_cache;
    bool _shutdown_cache;
    boost::mutex _mutex;
    size_t _div;
    size_t _divider_cache;
};

spi_core_3000::sptr spi_core_3000::make(wb_iface::sptr iface, const size_t base, const size_t readback)
{
    return sptr(new spi_core_3000_impl(iface, base, readback));
}

