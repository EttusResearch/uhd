//
// Copyright 2015 Ettus Research LLC
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

#include <cmath>

#include <gps.h>

#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread.hpp>

#include <uhd/exception.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/types/dict.hpp>

#include "gpsd_iface.hpp"

namespace uhd { namespace usrp {

static const size_t TIMEOUT = 240;
static const size_t CLICK_RATE = 250000;

class gpsd_iface_impl : public virtual gpsd_iface {
public:
    gpsd_iface_impl(const std::string &addr, boost::uint16_t port)
        : _detected(false), _bthread(), _timeout_cnt(0)
    {
        boost::unique_lock<boost::shared_mutex> l(_d_mutex);

        if (gps_open(addr.c_str(),
            str(boost::format("%u") % port).c_str(),
            &_gps_data) < 0) {
            throw uhd::runtime_error(
                str((boost::format("Failed to connect to gpsd: %s")
                    % gps_errstr(errno))));
        }

        // register for updates, we don't specify a specific device,
        // therefore no WATCH_DEVICE
        gps_stream(&_gps_data, WATCH_ENABLE, NULL);

        // create background thread talking to gpsd
        boost::thread t(boost::bind(&gpsd_iface_impl::_thread_fcn ,this));
        _bthread.swap(t);


        _sensors = boost::assign::list_of("gps_locked")("gps_time")("gps_position");
    }

    virtual ~gpsd_iface_impl(void)
    {
        // interrupt the background thread and wait for it to finish
        _bthread.interrupt();
        _bthread.join();

        // clean up ...
        {
            boost::unique_lock<boost::shared_mutex> l(_d_mutex);

            gps_stream(&_gps_data, WATCH_DISABLE, NULL);
            gps_close(&_gps_data);
        }
    }

    uhd::sensor_value_t get_sensor(std::string key)
    {
        if (key == "gps_locked") {
            return sensor_value_t(
                "GPS lock status", _gps_locked(), "locked", "unlocked");
        } else if (key == "gps_time") {
            return sensor_value_t(
                "GPS epoch time", int(_epoch_time()), "seconds");
        } else if (key == "gps_position") {
            return sensor_value_t(
                "GPS Position", str(
                    boost::format("%s %s %s")
                    % _gps_position()["lat"]
                    % _gps_position()["lon"]
                    % _gps_position()["alt"]), "lat/lon/alt");
        } else
            throw uhd::key_error(
                str(boost::format("sensor %s unknown.") % key));
    }

    bool gps_detected(void) { return _detected; };

    std::vector<std::string> get_sensors(void) { return _sensors; };

private: // member functions
    void _thread_fcn()
    {
        while (not boost::this_thread::interruption_requested()) {
            if (!gps_waiting(&_gps_data, CLICK_RATE)) {
                if (TIMEOUT < _timeout_cnt++)
                    _detected = false;
            } else {
                boost::unique_lock<boost::shared_mutex> l(_d_mutex);

                _timeout_cnt = 0;
                _detected = true;

                if (gps_read(&_gps_data) < 0)
                    throw std::runtime_error("error while reading");
            }
        }
    }

    bool _gps_locked(void)
    {
        boost::shared_lock<boost::shared_mutex> l(_d_mutex);
        return _gps_data.fix.mode >= MODE_2D;
    }

    std::time_t _epoch_time(void)
    {
        boost::shared_lock<boost::shared_mutex> l(_d_mutex);
        return (boost::posix_time::from_time_t(_gps_data.fix.time)
            - boost::posix_time::from_time_t(0)).total_seconds();
    }

    boost::gregorian::date _gregorian_date(void)
    {
        boost::shared_lock<boost::shared_mutex> l(_d_mutex);
        return boost::posix_time::from_time_t(_gps_data.fix.time).date();
    }

    uhd::dict<std::string, std::string> _gps_position(void)
    {
        boost::shared_lock<boost::shared_mutex> l(_d_mutex);

        uhd::dict<std::string, std::string> tmp;
        if (_gps_data.fix.mode >= MODE_2D) {
            tmp["lon"] = str(boost::format("%f deg")
                    % _gps_data.fix.longitude);
            tmp["lat"] = str(boost::format("%f deg")
                    % _gps_data.fix.latitude);
            tmp["alt"] = str(boost::format("%fm")
                    % _gps_data.fix.altitude);
        } else {
            tmp["lon"] = "n/a";
            tmp["lat"] = "n/a";
            tmp["alt"] = "n/a";
        }
        return tmp;
    }

private: // members
    std::vector<std::string> _sensors;
    bool                     _detected;

    gps_data_t               _gps_data;
    boost::shared_mutex      _d_mutex;
    boost::thread            _bthread;
    size_t                   _timeout_cnt;
};

}} //namespace

using namespace uhd::usrp;

gpsd_iface::sptr gpsd_iface::make(const std::string &addr, const boost::uint16_t port)
{
    return gpsd_iface::sptr(new gpsd_iface_impl(addr, port));
}
