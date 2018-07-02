//
// Copyright 2012-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/ranges.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/types/serial.hpp>
#include <uhdlib/usrp/common/ad9361_ctrl.hpp>
#include <boost/format.hpp>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <cstring>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * AD9361 IO Implementation Classes
 **********************************************************************/

class ad9361_io_spi : public ad9361_io
{
public:
    ad9361_io_spi(uhd::spi_iface::sptr spi_iface, uint32_t slave_num) :
        _spi_iface(spi_iface), _slave_num(slave_num) { }

    virtual ~ad9361_io_spi() { }

    virtual uint8_t peek8(uint32_t reg)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        uhd::spi_config_t config;
        config.mosi_edge = uhd::spi_config_t::EDGE_FALL;
        config.miso_edge = uhd::spi_config_t::EDGE_FALL;    //TODO (Ashish): FPGA SPI workaround. This should be EDGE_RISE

        uint32_t rd_word = AD9361_SPI_READ_CMD |
                           ((uint32_t(reg) << AD9361_SPI_ADDR_SHIFT) & AD9361_SPI_ADDR_MASK);

        uint32_t val = (_spi_iface->read_spi(_slave_num, config, rd_word, AD9361_SPI_NUM_BITS));
        val &= 0xFF;

        return static_cast<uint8_t>(val);
    }

    virtual void poke8(uint32_t reg, uint8_t val)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        uhd::spi_config_t config;
        config.mosi_edge = uhd::spi_config_t::EDGE_FALL;
        config.miso_edge = uhd::spi_config_t::EDGE_FALL;    //TODO (Ashish): FPGA SPI workaround. This should be EDGE_RISE

        uint32_t wr_word = AD9361_SPI_WRITE_CMD |
                           ((uint32_t(reg) << AD9361_SPI_ADDR_SHIFT) & AD9361_SPI_ADDR_MASK) |
                           ((uint32_t(val) << AD9361_SPI_DATA_SHIFT) & AD9361_SPI_DATA_MASK);
        _spi_iface->write_spi(_slave_num, config, wr_word, AD9361_SPI_NUM_BITS);
    }

private:
    uhd::spi_iface::sptr    _spi_iface;
    uint32_t         _slave_num;
    boost::mutex            _mutex;

    static const uint32_t AD9361_SPI_WRITE_CMD  = 0x00800000;
    static const uint32_t AD9361_SPI_READ_CMD   = 0x00000000;
    static const uint32_t AD9361_SPI_ADDR_MASK  = 0x003FFF00;
    static const uint32_t AD9361_SPI_ADDR_SHIFT = 8;
    static const uint32_t AD9361_SPI_DATA_MASK  = 0x000000FF;
    static const uint32_t AD9361_SPI_DATA_SHIFT = 0;
    static const uint32_t AD9361_SPI_NUM_BITS   = 24;
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
        double return_val = _device.set_gain(direction, chain, value);
        return return_val;
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
            UHD_LOGGER_WARNING("AD936X") << boost::format(
                    "The requested master_clock_rate %f MHz exceeds bounds imposed by UHD.\n"
                    "The master_clock_rate has been forced to %f MHz.\n"
            ) % (rate/1e6) % (clipped_rate/1e6) ;
        }

        double return_rate = _device.set_clock_rate(clipped_rate);

        return return_rate;
    }

    //! set which RX and TX chains/antennas are active
    void set_active_chains(bool tx1, bool tx2, bool rx1, bool rx2)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        _device.set_active_chains(tx1, tx2, rx1, rx2);
    }

    //! set which timing mode to use - 1R1T, 2R2T
    void set_timing_mode(const std::string &timing_mode)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);

        if ((timing_mode != "2R2T") && (timing_mode != "1R1T")) {
            throw uhd::assertion_error("ad9361_ctrl: Timing mode not supported");
        }
        _device.set_timing_mode((timing_mode == "2R2T")? ad9361_device_t::TIMING_MODE_2R2T : ad9361_device_t::TIMING_MODE_1R1T);
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
        double return_val = _device.tune(direction, value);
        return return_val;
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
        ad9361_device_t::direction_t direction = _get_direction_from_antenna(which);
        double actual_bw = bw;

        {
            boost::lock_guard<boost::mutex> lock(_mutex);
            actual_bw = _device.set_bw_filter(direction, bw);
        }

        const double min_bw = ad9361_device_t::AD9361_MIN_BW;
        const double max_bw = ad9361_device_t::AD9361_MAX_BW;
        if (bw < min_bw or bw > max_bw)
        {
            UHD_LOGGER_WARNING("AD936X") << boost::format(
                    "The requested bandwidth %f MHz is out of range (%f - %f MHz).\n"
                    "The bandwidth has been forced to %f MHz.\n"
            ) % (bw/1e6) % (min_bw/1e6) % (max_bw/1e6) % (actual_bw/1e6);
        }
        return actual_bw;
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

    ad9361_device_t         _device;
    boost::mutex            _mutex;
};

//----------------------------------------------------------------------
// Make an instance of the AD9361 Control interface
//----------------------------------------------------------------------
ad9361_ctrl::sptr ad9361_ctrl::make_spi(
    ad9361_params::sptr client_settings,
    uhd::spi_iface::sptr spi_iface,
    uint32_t slave_num
) {
    boost::shared_ptr<ad9361_io_spi> spi_io_iface = boost::make_shared<ad9361_io_spi>(spi_iface, slave_num);
    return sptr(new ad9361_ctrl_impl(client_settings, spi_io_iface));
}
