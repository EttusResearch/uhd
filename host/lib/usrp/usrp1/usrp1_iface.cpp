//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "usrp1_iface.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/format.hpp>
#include <stdexcept>
#include <iomanip>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

class usrp1_iface_impl : public usrp1_iface{
public:
    /*******************************************************************
     * Structors
     ******************************************************************/
    usrp1_iface_impl(uhd::usrp::fx2_ctrl::sptr ctrl_transport)
    {
        _ctrl_transport = ctrl_transport;
    }

    ~usrp1_iface_impl(void)
    {
        /* NOP */
    }

    /*******************************************************************
     * Peek and Poke
     ******************************************************************/
    void poke32(const uint32_t addr, const uint32_t value)
    {
        uint32_t swapped = uhd::htonx(value);

        UHD_LOGGER_TRACE("USRP1")
            << "poke32("
            << std::dec << std::setw(2) << addr << ", 0x"
            << std::hex << std::setw(8) << value << ")"
        ;

        uint8_t w_index_h = SPI_ENABLE_FPGA & 0xff;
        uint8_t w_index_l = (SPI_FMT_MSB | SPI_FMT_HDR_1) & 0xff;

        int ret =_ctrl_transport->usrp_control_write(
                                          VRQ_SPI_WRITE,
                                          addr & 0x7f,
                                          (w_index_h << 8) | (w_index_l << 0),
                                          (unsigned char*) &swapped,
                                          sizeof(uint32_t));

        if (ret < 0) throw uhd::io_error("USRP1: failed control write");
    }

    uint32_t peek32(const uint32_t addr)
    {
        UHD_LOGGER_TRACE("USRP1")
            << "peek32("
            << std::dec << std::setw(2) << addr << ")"
        ;

        uint32_t value_out;

        uint8_t w_index_h = SPI_ENABLE_FPGA & 0xff;
        uint8_t w_index_l = (SPI_FMT_MSB | SPI_FMT_HDR_1) & 0xff;

        int ret = _ctrl_transport->usrp_control_read(
                                          VRQ_SPI_READ,
                                          0x80 | (addr & 0x7f),
                                          (w_index_h << 8) | (w_index_l << 0),
                                          (unsigned char*) &value_out,
                                          sizeof(uint32_t));

        if (ret < 0) throw uhd::io_error("USRP1: failed control read");

        return uhd::ntohx(value_out);
    }

    void poke16(const uint32_t, const uint16_t) {
        throw uhd::not_implemented_error("Unhandled command poke16()");
    }

    uint16_t peek16(const uint32_t) {
        throw uhd::not_implemented_error("Unhandled command peek16()");
        return 0;
    }

    /*******************************************************************
     * I2C
     ******************************************************************/
    void write_i2c(uint16_t addr, const byte_vector_t &bytes){
        return _ctrl_transport->write_i2c(addr, bytes);
    }

    byte_vector_t read_i2c(uint16_t addr, size_t num_bytes){
        return _ctrl_transport->read_i2c(addr, num_bytes);
    }

    /*******************************************************************
     * SPI
     *
     * For non-readback transactions use the SPI_WRITE command, which is
     * simpler and uses the USB control buffer for OUT data. No data
     * needs to be returned.
     *
     * For readback transactions use SPI_TRANSACT, which places up to
     * 4 bytes of OUT data in the device request fields and uses the
     * control buffer for IN data.
     ******************************************************************/
    uint32_t transact_spi(int which_slave,
                                 const spi_config_t &,
                                 uint32_t bits,
                                 size_t num_bits,
                                 bool readback)
    {
        UHD_LOGGER_TRACE("USRP1")
            << "transact_spi: "
            << "  slave: " << which_slave
            << "  bits: " << bits
            << "  num_bits: " << num_bits
            << "  readback: " << readback
        ;
        UHD_ASSERT_THROW((num_bits <= 32) && !(num_bits % 8));
        size_t num_bytes = num_bits / 8;

        if (readback) {
            unsigned char buff[4] = {
                (unsigned char)(bits & 0xff),
                (unsigned char)((bits >> 8) & 0xff),
                (unsigned char)((bits >> 16) & 0xff),
                (unsigned char)((bits >> 24) & 0xff)
            };
            //conditions where there are two header bytes
            if (num_bytes >= 3 and buff[num_bytes-1] != 0 and buff[num_bytes-2] != 0 and buff[num_bytes-3] == 0){
                if (int(num_bytes-2) != _ctrl_transport->usrp_control_read(
                    VRQ_SPI_READ, (buff[num_bytes-1] << 8) | (buff[num_bytes-2] << 0),
                    (which_slave << 8) | SPI_FMT_MSB | SPI_FMT_HDR_2,
                    buff, num_bytes-2
                )) throw uhd::io_error("USRP1: failed SPI readback transaction");
            }

            //conditions where there is one header byte
            else if (num_bytes >= 2 and buff[num_bytes-1] != 0 and buff[num_bytes-2] == 0){
                if (int(num_bytes-1) != _ctrl_transport->usrp_control_read(
                    VRQ_SPI_READ, buff[num_bytes-1],
                    (which_slave << 8) | SPI_FMT_MSB | SPI_FMT_HDR_1,
                    buff, num_bytes-1
                )) throw uhd::io_error("USRP1: failed SPI readback transaction");
            }
            else{
                throw uhd::io_error("USRP1: invalid input data for SPI readback");
            }
            uint32_t val = (((uint32_t)buff[0]) <<  0) |
                                  (((uint32_t)buff[1]) <<  8) |
                                  (((uint32_t)buff[2]) << 16) |
                                  (((uint32_t)buff[3]) << 24);
            return val;
        }
        else {
            // Byteswap on num_bytes
            unsigned char buff[4] = { 0 };
            for (size_t i = 1; i <= num_bytes; i++)
                buff[num_bytes - i] = (bits >> ((i - 1) * 8)) & 0xff;

            uint8_t w_index_h = which_slave & 0xff;
            uint8_t w_index_l = (SPI_FMT_MSB | SPI_FMT_HDR_0) & 0xff;

            int ret =_ctrl_transport->usrp_control_write(
                                          VRQ_SPI_WRITE,
                                          0x00,
                                          (w_index_h << 8) | (w_index_l << 0),
                                          buff, num_bytes);

            if (ret < 0) throw uhd::io_error("USRP1: failed SPI transaction");

            return 0;
        }
    }

private:
    uhd::usrp::fx2_ctrl::sptr _ctrl_transport;
};

/***********************************************************************
 * Public Make Function
 **********************************************************************/
usrp1_iface::sptr usrp1_iface::make(uhd::usrp::fx2_ctrl::sptr ctrl_transport)
{
    return sptr(new usrp1_iface_impl(ctrl_transport));
}
