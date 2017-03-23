#include <mpm/lmk04828/lmk04828_spi_iface.hpp>
#include <uhd/exception.hpp>
#include <boost/make_shared.hpp>
#include <functional>

lmk04828_spi_iface::lmk04828_spi_iface(uhd::spi_iface::sptr iface) : _spi_iface(iface)
    {
        // Use default SPI Config options
        config = uhd::spi_config_t(uhd::spi_config_t::EDGE_RISE);
    }

lmk04828_iface::write_fn_t lmk04828_spi_iface::get_write_fn()
    {
        return std::bind(&lmk04828_spi_iface::spi_write, this, std::placeholders::_1);
    }

lmk04828_iface::read_fn_t lmk04828_spi_iface::get_read_fn()
    {
        return std::bind(&lmk04828_spi_iface::spi_read, this, std::placeholders::_1);
    }

void lmk04828_spi_iface::spi_write(std::vector<uint32_t> writes) {
        for (uint32_t write : writes) {
            _spi_iface->write_spi(DEFAULT_SLAVE, config, write, LMK_SPI_NUM_BITS);
        }
    }

uint8_t lmk04828_spi_iface::spi_read(uint32_t addr) {
        // Format LMK SPI read transaction
        // r/w[23] 0[22:21] addr[20:8] data[7:0] = 24 bits
        uint32_t transaction = 0;
        transaction |= LMK_SPI_READ_FLAG << LMK_SPI_READ_FLAG_OFFSET;
        transaction &= LMK_SPI_RESERVED_FIELD_MASK;
        transaction |= addr << LMK_SPI_READ_ADDR_OFFSET;

        uint32_t data = _spi_iface->read_spi(DEFAULT_SLAVE, config, transaction, LMK_SPI_NUM_BITS);

        if ((data & 0xFFFFFF00) != 0) {
            // There's more than 8 bits of data!
            throw uhd::runtime_error("LMK SPI read returned too much data");
        }

        return data & 0xFF;
    }

lmk04828_spi_iface::sptr lmk04828_spi_iface::make(uhd::spi_iface::sptr iface){
    return boost::make_shared<lmk04828_spi_iface>(iface);
}
