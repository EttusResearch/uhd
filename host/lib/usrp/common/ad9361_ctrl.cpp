//
// Copyright 2014 Ettus Research LLC
//

#include "ad9361_ctrl.hpp"
#include "ad9361_transaction.h"
#include "ad9361_dispatch.h"
#include <ad9361_platform.h>
#include <uhd/exception.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/types/serial.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/format.hpp>
#include <cstring>
#include <boost/utility.hpp>
#include <boost/function.hpp>

using namespace uhd;

/***********************************************************************
 * AD9361 IO Implementation Classes
 **********************************************************************/

class ad9361_io_spi : public ad9361_io
{
public:
    ad9361_io_spi(uhd::spi_iface::sptr spi_iface, uint32_t slave_num) :
        _spi_iface(spi_iface), _slave_num(slave_num) { }

    uint8_t peek8(uint32_t reg)
    {
        uhd::spi_config_t config;
        config.mosi_edge = uhd::spi_config_t::EDGE_FALL;
        config.miso_edge = uhd::spi_config_t::EDGE_FALL;    //TODO (Ashish): FPGA SPI workaround. This should be EDGE_RISE

        uint32_t rd_word = AD9361_SPI_READ_CMD |
                           ((uint32_t(reg) << AD9361_SPI_ADDR_SHIFT) & AD9361_SPI_ADDR_MASK);

        uint32_t val = (_spi_iface->read_spi(_slave_num, config, rd_word, AD9361_SPI_NUM_BITS));
        val &= 0xFF;

        return static_cast<uint8_t>(val);
    }

    void poke8(uint32_t reg, uint8_t val)
    {
        uhd::spi_config_t config;
        config.mosi_edge = uhd::spi_config_t::EDGE_FALL;
        config.miso_edge = uhd::spi_config_t::EDGE_FALL;    //TODO (Ashish): FPGA SPI workaround. This should be EDGE_RISE

        uint32_t wr_word = AD9361_SPI_WRITE_CMD |
                           ((uint32_t(reg) << AD9361_SPI_ADDR_SHIFT) & AD9361_SPI_ADDR_MASK) |
                           ((uint32_t(val) << AD9361_SPI_DATA_SHIFT) & AD9361_SPI_DATA_MASK);
        _spi_iface->write_spi(_slave_num, config, wr_word, AD9361_SPI_NUM_BITS);

        //TODO (Ashish): Is this necessary? The FX3 firmware does it right now but for
        //networked devices, it makes writes blocking which will considerably slow down the programming
        peek8(reg);
    }
private:
    uhd::spi_iface::sptr    _spi_iface;
    uint32_t                _slave_num;

    static const uint32_t AD9361_SPI_WRITE_CMD  = 0x00800000;
    static const uint32_t AD9361_SPI_READ_CMD   = 0x00000000;
    static const uint32_t AD9361_SPI_ADDR_MASK  = 0x003FFF00;
    static const uint32_t AD9361_SPI_ADDR_SHIFT = 8;
    static const uint32_t AD9361_SPI_DATA_MASK  = 0x000000FF;
    static const uint32_t AD9361_SPI_DATA_SHIFT = 0;
    static const uint32_t AD9361_SPI_NUM_BITS   = 24;
};

/***********************************************************************
 * AD9361 Transport Implementation Classes
 **********************************************************************/

//----------------------------------------------------------------------
//Over a zero-copy device transport
//----------------------------------------------------------------------
class ad9361_ctrl_transport_zc_impl : public ad9361_ctrl_transport
{
public:
    ad9361_ctrl_transport_zc_impl(uhd::transport::zero_copy_if::sptr xport)
    {
        _xport = xport;
    }

    void ad9361_transact(const unsigned char in_buff[AD9361_DISPATCH_PACKET_SIZE], unsigned char out_buff[AD9361_DISPATCH_PACKET_SIZE])
    {
        {
            uhd::transport::managed_send_buffer::sptr buff = _xport->get_send_buff(10.0);
            if (not buff or buff->size() < AD9361_DISPATCH_PACKET_SIZE) throw std::runtime_error("ad9361_ctrl_over_zc send timeout");
            std::memcpy(buff->cast<void *>(), in_buff, AD9361_DISPATCH_PACKET_SIZE);
            buff->commit(AD9361_DISPATCH_PACKET_SIZE);
        }
        {
            uhd::transport::managed_recv_buffer::sptr buff = _xport->get_recv_buff(10.0);
            if (not buff or buff->size() < AD9361_DISPATCH_PACKET_SIZE) throw std::runtime_error("ad9361_ctrl_over_zc recv timeout");
            std::memcpy(out_buff, buff->cast<const void *>(), AD9361_DISPATCH_PACKET_SIZE);
        }
    }

    uint64_t get_device_handle()
    {
        return 0;   //Unused for zero-copy transport because chip class is in FW
    }

private:
    uhd::transport::zero_copy_if::sptr _xport;
};

//----------------------------------------------------------------------
//Over a software transport
//----------------------------------------------------------------------
class ad9361_ctrl_transport_sw_spi_impl : public ad9361_ctrl_transport
{
public:
    ad9361_ctrl_transport_sw_spi_impl(
        ad9361_product_t product,
        uhd::spi_iface::sptr spi_iface,
        boost::uint32_t slave_num) :
        _io_iface(spi_iface, slave_num)
    {
        _device.product = product;
        _device.io_iface = reinterpret_cast<void*>(&_io_iface);
    }

    void ad9361_transact(const unsigned char in_buff[AD9361_DISPATCH_PACKET_SIZE], unsigned char out_buff[AD9361_DISPATCH_PACKET_SIZE])
    {
        ad9361_dispatch((const char*)in_buff, (char*)out_buff);
    }

    uint64_t get_device_handle()
    {
        return reinterpret_cast<uint64_t>(reinterpret_cast<void*>(&_device));
    }

private:
    ad9361_device_t _device;
    ad9361_io_spi   _io_iface;
};

//----------------------------------------------------------------------
// Make an instance of the AD9361 Transport
//----------------------------------------------------------------------
ad9361_ctrl_transport::sptr ad9361_ctrl_transport::make_zero_copy(uhd::transport::zero_copy_if::sptr xport)
{
    return sptr(new ad9361_ctrl_transport_zc_impl(xport));
}

ad9361_ctrl_transport::sptr ad9361_ctrl_transport::make_software_spi(
    ad9361_product_t product,
    uhd::spi_iface::sptr spi_iface,
    boost::uint32_t slave_num)
{
    return sptr(new ad9361_ctrl_transport_sw_spi_impl(product, spi_iface, slave_num));
}

/***********************************************************************
 * AD9361 Software API Class
 **********************************************************************/
class ad9361_ctrl_impl : public ad9361_ctrl
{
public:
    ad9361_ctrl_impl(ad9361_ctrl_transport::sptr iface):
        _iface(iface), _seq(0)
    {
        ad9361_transaction_t request;

        request.action = AD9361_ACTION_ECHO;
        this->do_transaction(request);

        request.action = AD9361_ACTION_INIT;
        this->do_transaction(request);
    }

    double set_gain(const std::string &which, const double value)
    {
        ad9361_transaction_t request;

        if (which == "RX1") request.action = AD9361_ACTION_SET_RX1_GAIN;
        if (which == "RX2") request.action = AD9361_ACTION_SET_RX2_GAIN;
        if (which == "TX1") request.action = AD9361_ACTION_SET_TX1_GAIN;
        if (which == "TX2") request.action = AD9361_ACTION_SET_TX2_GAIN;

        ad9361_double_pack(value, request.value.gain);
        const ad9361_transaction_t reply = this->do_transaction(request);
        return ad9361_double_unpack(reply.value.gain);
    }

    //! set a new clock rate, return the exact value
    double set_clock_rate(const double rate)
    {
        //warning for known trouble rates
        if (rate > 56e6) UHD_MSG(warning) << boost::format(
            "The requested clock rate %f MHz may cause slow configuration.\n"
            "The driver recommends a master clock rate less than %f MHz.\n"
        ) % (rate/1e6) % 56.0 << std::endl;

        //clip to known bounds
        const meta_range_t clock_rate_range = ad9361_ctrl::get_clock_rate_range();
        const double clipped_rate = clock_rate_range.clip(rate);

        ad9361_transaction_t request;
        request.action = AD9361_ACTION_SET_CLOCK_RATE;
        ad9361_double_pack(clipped_rate, request.value.rate);
        const ad9361_transaction_t reply = this->do_transaction(request);
        return ad9361_double_unpack(reply.value.rate);
    }

    //! set which RX and TX chains/antennas are active
    void set_active_chains(bool tx1, bool tx2, bool rx1, bool rx2)
    {
        boost::uint32_t mask = 0;
        if (tx1) mask |= (1 << 0);
        if (tx2) mask |= (1 << 1);
        if (rx1) mask |= (1 << 2);
        if (rx2) mask |= (1 << 3);

        ad9361_transaction_t request;
        request.action = AD9361_ACTION_SET_ACTIVE_CHAINS;
        request.value.enable_mask = mask;
        this->do_transaction(request);
    }

    //! tune the given frontend, return the exact value
    double tune(const std::string &which, const double freq)
    {
        //clip to known bounds
        const meta_range_t freq_range = ad9361_ctrl::get_rf_freq_range();
        const double clipped_freq = freq_range.clip(freq);

        ad9361_transaction_t request;

        if (which[0] == 'R') request.action = AD9361_ACTION_SET_RX_FREQ;
        if (which[0] == 'T') request.action = AD9361_ACTION_SET_TX_FREQ;

        const double value = ad9361_ctrl::get_rf_freq_range().clip(clipped_freq);
        ad9361_double_pack(value, request.value.freq);
        const ad9361_transaction_t reply = this->do_transaction(request);
        return ad9361_double_unpack(reply.value.freq);
    }

    //! turn on/off Catalina's data port loopback
    void data_port_loopback(const bool on)
    {
        ad9361_transaction_t request;
        request.action = AD9361_ACTION_SET_CODEC_LOOP;
        request.value.codec_loop = on? 1 : 0;
        this->do_transaction(request);
    }

    ad9361_transaction_t do_transaction(const ad9361_transaction_t &request)
    {
        boost::mutex::scoped_lock lock(_mutex);

        //declare in/out buffers
        unsigned char in_buff[AD9361_DISPATCH_PACKET_SIZE] = {};
        unsigned char out_buff[AD9361_DISPATCH_PACKET_SIZE] = {};

        //copy the input transaction
        std::memcpy(in_buff, &request, sizeof(request));

        //fill in other goodies
        ad9361_transaction_t *in = (ad9361_transaction_t *)in_buff;
        in->handle = _iface->get_device_handle();
        in->version = AD9361_TRANSACTION_VERSION;
        in->sequence = _seq++;

        //initialize error message to "no error"
        std::memset(in->error_msg, 0, AD9361_TRANSACTION_MAX_ERROR_MSG);

        //transact
        _iface->ad9361_transact(in_buff, out_buff);
        ad9361_transaction_t *out = (ad9361_transaction_t *)out_buff;

        //sanity checks
        UHD_ASSERT_THROW(out->version == in->version);
        UHD_ASSERT_THROW(out->sequence == in->sequence);

        //handle errors
        const size_t len = my_strnlen(out->error_msg, AD9361_TRANSACTION_MAX_ERROR_MSG);
        const std::string error_msg(out->error_msg, len);
        if (not error_msg.empty()) throw uhd::runtime_error("[ad9361_ctrl::do_transaction] firmware reported: \"" + error_msg + "\"");

        //return result done!
        return *out;
    }

private:
    //! compat strnlen for platforms that dont have it
    static size_t my_strnlen(const char *str, size_t max)
    {
        const char *end = (const char *)std::memchr((const void *)str, 0, max);
        if (end == NULL) return max;
        return (size_t)(end - str);
    }

    ad9361_ctrl_transport::sptr _iface;
    size_t                      _seq;
    boost::mutex                _mutex;
};

//----------------------------------------------------------------------
// Make an instance of the AD9361 Control interface
//----------------------------------------------------------------------
ad9361_ctrl::sptr ad9361_ctrl::make(ad9361_ctrl_transport::sptr iface)
{
    return sptr(new ad9361_ctrl_impl(iface));
}
