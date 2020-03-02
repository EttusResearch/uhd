//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <mpm/ad9361/e320_defaults.hpp>
#include <mpm/dboards/neon_manager.hpp>
#include <mpm/spi/spi_iface.hpp>
#include <mpm/spi/spi_regs_iface.hpp>
#include <mpm/types/regs_iface.hpp>
#include <memory>

using namespace mpm::dboards;
using namespace mpm::chips;
using namespace mpm::types;
using namespace mpm::types::e320;

namespace { /*anon*/

constexpr uint32_t AD9361_SPI_WRITE_CMD  = 0x00800000;
constexpr uint32_t AD9361_SPI_READ_CMD   = 0x00000000;
constexpr uint32_t AD9361_SPI_ADDR_MASK  = 0x003FFF00;
constexpr uint32_t AD9361_SPI_ADDR_SHIFT = 8;
constexpr uint32_t AD9361_SPI_DATA_MASK  = 0x000000FF;
constexpr uint32_t AD9361_SPI_DATA_SHIFT = 0;
constexpr uint32_t AD9361_SPI_NUM_BITS   = 24;
constexpr uint32_t AD9361_SPI_SPEED_HZ   = 2000000;
constexpr int AD9361_SPI_MODE            = 1;

} // namespace

/*! MPM-style E320 SPI Iface for AD9361 CTRL
 *
 */
class e320_ad9361_io_spi : public ad9361_io
{
public:
    e320_ad9361_io_spi(regs_iface::sptr regs_iface, uint32_t slave_num)
        : _regs_iface(regs_iface), _slave_num(slave_num)
    {
    }

    ~e320_ad9361_io_spi()
    { /*nop*/
    }

    uint8_t peek8(uint32_t reg)
    {
        return _regs_iface->peek8(reg);
    }

    void poke8(uint32_t reg, uint8_t val)
    {
        _regs_iface->poke8(reg, val);
    }

private:
    regs_iface::sptr _regs_iface;
    uint32_t _slave_num;
};

neon_manager::neon_manager(const std::string& catalina_spidev)
{
    // Make the MPM-style low level SPI Regs iface
    auto spi_iface = mpm::spi::make_spi_regs_iface(
        mpm::spi::spi_iface::make_spidev(
            catalina_spidev, AD9361_SPI_SPEED_HZ, AD9361_SPI_MODE),
        AD9361_SPI_ADDR_SHIFT,
        AD9361_SPI_DATA_SHIFT,
        AD9361_SPI_READ_CMD,
        AD9361_SPI_WRITE_CMD);
    // Make the SPI interface
    auto spi_io_iface = std::make_shared<e320_ad9361_io_spi>(spi_iface, 0);
    // Make the actual Catalina Ctrl object
    _catalina_ctrl =
        ad9361_ctrl::make_spi(std::make_shared<e320_ad9361_client_t>(), spi_io_iface);
}
