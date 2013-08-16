//
// Copyright 2012-2013 Ettus Research LLC
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

#ifndef INCLUDED_AD9361_CTRL_HPP
#define INCLUDED_AD9361_CTRL_HPP

#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <vector>
#include <string>


struct ad9361_ctrl_iface_type
{
    virtual void ad9361_transact(const unsigned char in_buff[64], unsigned char out_buff[64]) = 0;
};
typedef boost::shared_ptr<ad9361_ctrl_iface_type> ad9361_ctrl_iface_sptr;


struct ad9361_ctrl_over_zc : ad9361_ctrl_iface_type
{
    ad9361_ctrl_over_zc(uhd::transport::zero_copy_if::sptr xport)
    {
        _xport = xport;
    }

    void ad9361_transact(const unsigned char in_buff[64], unsigned char out_buff[64])
    {
        {
            uhd::transport::managed_send_buffer::sptr buff = _xport->get_send_buff(10.0);
            if (not buff or buff->size() < 64) throw std::runtime_error("ad9361_ctrl_over_zc send timeout");
            std::memcpy(buff->cast<void *>(), in_buff, 64);
            buff->commit(64);
        }
        {
            uhd::transport::managed_recv_buffer::sptr buff = _xport->get_recv_buff(10.0);
            if (not buff or buff->size() < 64) throw std::runtime_error("ad9361_ctrl_over_zc recv timeout");
            std::memcpy(out_buff, buff->cast<const void *>(), 64);
        }
    }

    uhd::transport::zero_copy_if::sptr _xport;
};


class ad9361_ctrl : boost::noncopyable{
public:
    typedef boost::shared_ptr<ad9361_ctrl> sptr;

    //! make a new codec control object
    static sptr make(ad9361_ctrl_iface_sptr iface);

    //! Get a list of gain names for RX or TX
    static std::vector<std::string> get_gain_names(const std::string &/*which*/)
    {
        return std::vector<std::string>(1, "PGA");
    }

    //! get the gain range for a particular gain element
    static uhd::meta_range_t get_gain_range(const std::string &which)
    {
        if(which[0] == 'R') {
            return uhd::meta_range_t(0.0, 73.0, 1.0);
        } else {
            return uhd::meta_range_t(0.0, 89.75, 0.25);
        }
    }

    //! get the freq range for the frontend which
    static uhd::meta_range_t get_rf_freq_range(void)
    {
        return uhd::meta_range_t(50e6, 6e9);
    }

    //! get the filter range for the frontend which
    static uhd::meta_range_t get_bw_filter_range(const std::string &/*which*/)
    {
        return uhd::meta_range_t(200e3, 56e6);
    }

    //! get the clock rate range for the frontend
    static uhd::meta_range_t get_clock_rate_range(void)
    {
        //return uhd::meta_range_t(220e3, 61.44e6);
        return uhd::meta_range_t(5e6, 61.44e6); //5 MHz DCM low end
    }

    //! set the filter bandwidth for the frontend
    double set_bw_filter(const std::string &/*which*/, const double /*bw*/)
    {
        return 56e6; //TODO
    }

    //! set the gain for a particular gain element
    virtual double set_gain(const std::string &which, const double value) = 0;

    //! set a new clock rate, return the exact value
    virtual double set_clock_rate(const double rate) = 0;

    //! set which RX and TX chains/antennas are active
    virtual void set_active_chains(bool tx1, bool tx2, bool rx1, bool rx2) = 0;

    //! tune the given frontend, return the exact value
    virtual double tune(const std::string &which, const double value) = 0;

    //! turn on/off Catalina's data port loopback
    virtual void data_port_loopback(const bool on) = 0;
};

#endif /* INCLUDED_AD9361_CTRL_HPP */
