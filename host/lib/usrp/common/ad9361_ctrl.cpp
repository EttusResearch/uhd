//
// Copyright 2012-2015 Ettus Research LLC
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

#include "ad9361_ctrl.hpp"
#include <uhd/types/ranges.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/types/serial.hpp>
#include <cstring>
#include <boost/format.hpp>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * AD9361 IO Implementation Classes
 **********************************************************************/

class ad9361_io_spi : public ad9361_io
{
public:
    ad9361_io_spi(uhd::spi_iface::sptr spi_iface, boost::uint32_t slave_num) :
        _spi_iface(spi_iface), _slave_num(slave_num) { }

    virtual ~ad9361_io_spi() { }

    virtual boost::uint8_t peek8(boost::uint32_t reg)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        uhd::spi_config_t config;
        config.mosi_edge = uhd::spi_config_t::EDGE_FALL;
        config.miso_edge = uhd::spi_config_t::EDGE_FALL;    //TODO (Ashish): FPGA SPI workaround. This should be EDGE_RISE

        boost::uint32_t rd_word = AD9361_SPI_READ_CMD |
                           ((boost::uint32_t(reg) << AD9361_SPI_ADDR_SHIFT) & AD9361_SPI_ADDR_MASK);

        boost::uint32_t val = (_spi_iface->read_spi(_slave_num, config, rd_word, AD9361_SPI_NUM_BITS));
        val &= 0xFF;

        return static_cast<boost::uint8_t>(val);
    }

    virtual void poke8(boost::uint32_t reg, boost::uint8_t val)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        uhd::spi_config_t config;
        config.mosi_edge = uhd::spi_config_t::EDGE_FALL;
        config.miso_edge = uhd::spi_config_t::EDGE_FALL;    //TODO (Ashish): FPGA SPI workaround. This should be EDGE_RISE

        boost::uint32_t wr_word = AD9361_SPI_WRITE_CMD |
                           ((boost::uint32_t(reg) << AD9361_SPI_ADDR_SHIFT) & AD9361_SPI_ADDR_MASK) |
                           ((boost::uint32_t(val) << AD9361_SPI_DATA_SHIFT) & AD9361_SPI_DATA_MASK);
        _spi_iface->write_spi(_slave_num, config, wr_word, AD9361_SPI_NUM_BITS);
    }

private:
    uhd::spi_iface::sptr    _spi_iface;
    boost::uint32_t         _slave_num;
    boost::mutex            _mutex;

    static const boost::uint32_t AD9361_SPI_WRITE_CMD  = 0x00800000;
    static const boost::uint32_t AD9361_SPI_READ_CMD   = 0x00000000;
    static const boost::uint32_t AD9361_SPI_ADDR_MASK  = 0x003FFF00;
    static const boost::uint32_t AD9361_SPI_ADDR_SHIFT = 8;
    static const boost::uint32_t AD9361_SPI_DATA_MASK  = 0x000000FF;
    static const boost::uint32_t AD9361_SPI_DATA_SHIFT = 0;
    static const boost::uint32_t AD9361_SPI_NUM_BITS   = 24;
};

/***********************************************************************
 * AD9361 Control API Class
 **********************************************************************/
class ad9361_ctrl_impl : public ad9361_ctrl
{
public:
    ad9361_ctrl_impl(ad9361_params::sptr client_settings, ad9361_io::sptr io_iface):
        _device(client_settings, io_iface)
    {
        _device.initialize();
    }

    double set_gain(const std::string &which, const double value)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        ad9361_device_t::direction_t direction = _get_direction_from_antenna(which);
        ad9361_device_t::chain_t chain =_get_chain_from_antenna(which);
        return _device.set_gain(direction, chain, value);
    }

    void set_agc(const std::string &which, bool enable)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        ad9361_device_t::chain_t chain =_get_chain_from_antenna(which);
         _device.set_agc(chain, enable);
    }

    void set_agc_mode(const std::string &which, const std::string &mode)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        ad9361_device_t::chain_t chain =_get_chain_from_antenna(which);
        if(mode == "slow") {
            _device.set_agc_mode(chain, ad9361_device_t::GAIN_MODE_SLOW_AGC);
        } else if (mode == "fast"){
            _device.set_agc_mode(chain, ad9361_device_t::GAIN_MODE_FAST_AGC);
        } else {
            throw uhd::runtime_error("ad9361_ctrl got an invalid AGC option.");
        }
    }

    //! set a new clock rate, return the exact value
    double set_clock_rate(const double rate)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        //clip to known bounds
        const meta_range_t clock_rate_range = ad9361_ctrl::get_clock_rate_range();
        const double clipped_rate = clock_rate_range.clip(rate);

        if (clipped_rate != rate) {
            UHD_MSG(warning) << boost::format(
                    "The requested master_clock_rate %f MHz exceeds bounds imposed by UHD.\n"
                    "The master_clock_rate has been forced to %f MHz.\n"
            ) % (rate/1e6) % (clipped_rate/1e6) << std::endl;
        }

        return _device.set_clock_rate(clipped_rate);
    }

    //! set which RX and TX chains/antennas are active
    void set_active_chains(bool tx1, bool tx2, bool rx1, bool rx2)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        _device.set_active_chains(tx1, tx2, rx1, rx2);
    }

    //! tune the given frontend, return the exact value
    double tune(const std::string &which, const double freq)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        //clip to known bounds
        const meta_range_t freq_range = ad9361_ctrl::get_rf_freq_range();
        const double clipped_freq = freq_range.clip(freq);
        const double value = ad9361_ctrl::get_rf_freq_range().clip(clipped_freq);

        ad9361_device_t::direction_t direction = _get_direction_from_antenna(which);
        return _device.tune(direction, value);
    }

    //! get the current frequency for the given frontend
    double get_freq(const std::string &which)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        ad9361_device_t::direction_t direction = _get_direction_from_antenna(which);
        return _device.get_freq(direction);
    }

    //! turn on/off data port loopback
    void data_port_loopback(const bool on)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        _device.data_port_loopback(on);
    }

    //! read internal RSSI sensor
    sensor_value_t get_rssi(const std::string &which)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        ad9361_device_t::chain_t chain =_get_chain_from_antenna(which);
        return sensor_value_t("RSSI", _device.get_rssi(chain), "dB");
    }

    //! read the internal temp sensor. Average over 3 results
    sensor_value_t get_temperature()
    {
        return sensor_value_t("temp", _device.get_average_temperature(), "C");
    }

    void set_dc_offset_auto(const std::string &which, const bool on)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        ad9361_device_t::direction_t direction = _get_direction_from_antenna(which);
        _device.set_dc_offset_auto(direction,on);
    }

    void set_iq_balance_auto(const std::string &which, const bool on)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        ad9361_device_t::direction_t direction = _get_direction_from_antenna(which);
        _device.set_iq_balance_auto(direction,on);
    }

    double set_bw_filter(const std::string &which, const double bw)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        ad9361_device_t::direction_t direction = _get_direction_from_antenna(which);
        return _device.set_bw_filter(direction, bw);
    }

    std::vector<std::string> get_filter_names(const std::string &which)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        ad9361_device_t::direction_t direction = _get_direction_from_antenna(which);
        return _device.get_filter_names(direction);
    }

    filter_info_base::sptr get_filter(const std::string &which, const std::string &filter_name)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        ad9361_device_t::direction_t direction = _get_direction_from_antenna(which);
        ad9361_device_t::chain_t chain =_get_chain_from_antenna(which);
        return _device.get_filter(direction, chain, filter_name);
    }

    void set_filter(const std::string &which, const std::string &filter_name, const filter_info_base::sptr filter)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        ad9361_device_t::direction_t direction = _get_direction_from_antenna(which);
        ad9361_device_t::chain_t chain = _get_chain_from_antenna(which);
        _device.set_filter(direction, chain, filter_name, filter);
    }

    void output_digital_test_tone(bool enb)
    {
        _device.digital_test_tone(enb);
    }

private:
    static ad9361_device_t::direction_t _get_direction_from_antenna(const std::string& antenna)
    {
        std::string sub = antenna.substr(0, 2);
        if (sub == "RX") {
            return ad9361_device_t::RX;
        } else if (sub == "TX") {
            return ad9361_device_t::TX;
        } else {
            throw uhd::runtime_error("ad9361_ctrl got an invalid channel string.");
        }
        return ad9361_device_t::RX;
    }

    static ad9361_device_t::chain_t _get_chain_from_antenna(const std::string& antenna)
    {
        std::string sub = antenna.substr(2, 1);
        if (sub == "1") {
            return ad9361_device_t::CHAIN_1;
        } else if (sub == "2") {
            return ad9361_device_t::CHAIN_2;
        } else {
            throw uhd::runtime_error("ad9361_ctrl::set_gain got an invalid channel string.");
        }
        return ad9361_device_t::CHAIN_1;
    }

    ad9361_device_t _device;
    boost::mutex    _mutex;
};

//----------------------------------------------------------------------
// Make an instance of the AD9361 Control interface
//----------------------------------------------------------------------
ad9361_ctrl::sptr ad9361_ctrl::make_spi(
    ad9361_params::sptr client_settings, uhd::spi_iface::sptr spi_iface, boost::uint32_t slave_num)
{
    boost::shared_ptr<ad9361_io_spi> spi_io_iface = boost::make_shared<ad9361_io_spi>(spi_iface, slave_num);
    return sptr(new ad9361_ctrl_impl(client_settings, spi_io_iface));
}
