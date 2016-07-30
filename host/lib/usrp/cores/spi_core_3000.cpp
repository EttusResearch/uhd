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

#include "spi_core_3000.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/msg.hpp>
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

    boost::uint32_t transact_spi(
        int which_slave,
        const spi_config_t &config,
        boost::uint32_t data,
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
        boost::uint32_t ctrl_word = 0;
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
        const boost::uint32_t data_out = data << (32 - num_bits);

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
    boost::uint32_t _ctrl_word_cache;
    bool _shutdown_cache;
    boost::mutex _mutex;
    size_t _div;
    size_t _divider_cache;
};

spi_core_3000::sptr spi_core_3000::make(wb_iface::sptr iface, const size_t base, const size_t readback)
{
    return sptr(new spi_core_3000_impl(iface, base, readback));
}

