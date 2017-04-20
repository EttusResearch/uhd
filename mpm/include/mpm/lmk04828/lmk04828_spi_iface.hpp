#pragma once

#include "lmk04828.hpp"
#include "uhd/types/serial.hpp"
#include <boost/shared_ptr.hpp>

class lmk04828_spi_iface
{
public:
    using sptr = boost::shared_ptr<lmk04828_spi_iface>;
    lmk04828_spi_iface(uhd::spi_iface::sptr iface);
    lmk04828_iface::write_fn_t get_write_fn();
    lmk04828_iface::read_fn_t get_read_fn();
    static sptr make(uhd::spi_iface::sptr iface);

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

#ifdef LIBMPM_PYTHON
void export_lmk(){
    LIBMPM_BOOST_PREAMBLE("lmk04828")
    bp::class_<lmk04828_iface, boost::shared_ptr<lmk04828_iface>, boost::noncopyable >("lmk04828_iface", bp::no_init)
        .def("verify_chip_id", &lmk04828_iface::verify_chip_id)
        .def("get_chip_id", &lmk04828_iface::get_chip_id)
        .def("init", &lmk04828_iface::init)
        .def("enable_sysref_pulse", &lmk04828_iface::enable_sysref_pulse)
   ;
}
//        .def("make", &lmk04828_iface::make)
#endif
