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

#include "n230_cores.hpp"
#include "n230_fpga_defs.h"
#include "n230_fw_defs.h"

namespace uhd { namespace usrp { namespace n230 {

n230_core_spi_core::n230_core_spi_core(
    uhd::wb_iface::sptr iface,
    perif_t default_perif) :
    _spi_core(spi_core_3000::make(iface,
                                  fpga::sr_addr(fpga::SR_CORE_SPI),
                                  fpga::rb_addr(fpga::RB_CORE_SPI))),
    _current_perif(default_perif),
    _last_perif(default_perif)
{
    change_perif(default_perif);
}

boost::uint32_t n230_core_spi_core::transact_spi(
    int which_slave,
    const spi_config_t &config,
    boost::uint32_t data,
    size_t num_bits,
    bool readback)
{
    boost::mutex::scoped_lock lock(_mutex);
    return _spi_core->transact_spi(which_slave, config, data, num_bits, readback);
}

void n230_core_spi_core::change_perif(perif_t perif)
{
    boost::mutex::scoped_lock lock(_mutex);
    _last_perif = _current_perif;
    _current_perif = perif;

    switch (_current_perif) {
        case CODEC:
            _spi_core->set_divider(fw::CPU_CLOCK_FREQ/fw::CODEC_SPI_CLOCK_FREQ);
            break;
        case PLL:
            _spi_core->set_divider(fw::CPU_CLOCK_FREQ/fw::ADF4001_SPI_CLOCK_FREQ);
            break;
    }
}

void n230_core_spi_core::restore_perif()
{
    change_perif(_last_perif);
}

n230_ref_pll_ctrl::n230_ref_pll_ctrl(n230_core_spi_core::sptr spi) :
    adf4001_ctrl(spi, fpga::ADF4001_SPI_SLAVE_NUM),
    _spi(spi)
{
}

void n230_ref_pll_ctrl::set_lock_to_ext_ref(bool external)
{
    _spi->change_perif(n230_core_spi_core::PLL);
    adf4001_ctrl::set_lock_to_ext_ref(external);
    _spi->restore_perif();
}

}}} //namespace

using namespace uhd::usrp::n230;
using namespace uhd::usrp;

n230_core_spi_core::sptr n230_core_spi_core::make(
    uhd::wb_iface::sptr iface, n230_core_spi_core::perif_t default_perif)
{
    return sptr(new n230_core_spi_core(iface, default_perif));
}

