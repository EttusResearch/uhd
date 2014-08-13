//
// Copyright 2011,2014 Ettus Research LLC
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

#include "spi_core_100.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/msg.hpp>
#include <boost/thread/thread.hpp> //sleep

#define REG_SPI_TXRX0 _base + 0
#define REG_SPI_TXRX1 _base + 4
#define REG_SPI_TXRX2 _base + 8
#define REG_SPI_TXRX3 _base + 12
#define REG_SPI_CTRL  _base + 16
#define REG_SPI_DIV   _base + 20
#define REG_SPI_SS    _base + 24

//spi ctrl register bit definitions
#define SPI_CTRL_ASS      (1<<13)
#define SPI_CTRL_IE       (1<<12)
#define SPI_CTRL_LSB      (1<<11)
#define SPI_CTRL_TXNEG    (1<<10) //mosi edge, push on falling edge when 1
#define SPI_CTRL_RXNEG    (1<< 9) //miso edge, latch on falling edge when 1
#define SPI_CTRL_GO_BSY   (1<< 8)
#define SPI_CTRL_CHAR_LEN_MASK 0x7F

using namespace uhd;

spi_core_100::~spi_core_100(void){
    /* NOP */
}

class spi_core_100_impl : public spi_core_100{
public:
    spi_core_100_impl(wb_iface::sptr iface, const size_t base):
        _iface(iface), _base(base) { /* NOP */}

    boost::uint32_t transact_spi(
        int which_slave,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits,
        bool readback
    ){
        UHD_ASSERT_THROW(num_bits <= 32 and (num_bits % 8) == 0);

        int edge_flags = ((config.miso_edge==spi_config_t::EDGE_FALL) ? SPI_CTRL_RXNEG : 0) |
                         ((config.mosi_edge==spi_config_t::EDGE_FALL) ? 0 : SPI_CTRL_TXNEG)
                         ;
        boost::uint16_t ctrl = SPI_CTRL_ASS | (SPI_CTRL_CHAR_LEN_MASK & num_bits) | edge_flags;

        spi_wait();
        _iface->poke16(REG_SPI_DIV, 0x0001); // = fpga_clk / 4
        _iface->poke32(REG_SPI_SS, which_slave & 0xFFFF);
        _iface->poke32(REG_SPI_TXRX0, data);
        _iface->poke16(REG_SPI_CTRL, ctrl);
        _iface->poke16(REG_SPI_CTRL, ctrl | SPI_CTRL_GO_BSY);

        if (not readback) return 0;
        spi_wait();
        return _iface->peek32(REG_SPI_TXRX0);
    }

private:
    void spi_wait(void) {
        for (size_t i = 0; i < 100; i++){
            if ((_iface->peek16(REG_SPI_CTRL) & SPI_CTRL_GO_BSY) == 0) return;
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }
        UHD_MSG(error) << "spi_core_100: spi_wait timeout" << std::endl;
    }

    wb_iface::sptr _iface;
    const size_t _base;
};

spi_core_100::sptr spi_core_100::make(wb_iface::sptr iface, const size_t base){
    return sptr(new spi_core_100_impl(iface, base));
}
