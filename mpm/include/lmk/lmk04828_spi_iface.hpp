#include "lmk04828.hpp"
#include "uhd/types/serial.hpp"

class lmk04828_spi_iface
{
public:
    lmk04828_spi_iface(uhd::spi_iface::sptr iface);
    lmk04828_iface::write_fn_t get_write_fn();
    lmk04828_iface::read_fn_t get_read_fn();

private:
    const int LMK_SPI_NUM_BITS = 24;
    const int LMK_SPI_READ_FLAG = 1;
    const int LMK_SPI_READ_FLAG_OFFSET = 23;
    const int LMK_SPI_READ_ADDR_OFFSET = 8;
    const int LMK_SPI_RESERVED_FIELD_MASK = ~(0x3 << 21);
    const int DEFAULT_SLAVE = 1;

    uhd::spi_iface::sptr _spi_iface;
    uhd::spi_config_t config;

    void spi_write(std::vector<uint32_t> writes);
    uint8_t spi_read(uint32_t addr);
};
