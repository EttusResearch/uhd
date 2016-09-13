//
// Copyright 2015-2016 Ettus Research LLC
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
#include <boost/math/special_functions/fpclassify.hpp>

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


        _sensors = boost::assign::list_of<std::string>("gps_locked")("gps_time") \
            ("gps_position")("gps_gpgga")("gps_gprmc").to_container(_sensors);
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
        } else if (key == "gps_gpgga") {
            return sensor_value_t(
                "GPGGA", _gps_gpgga(), "");
        } else if (key == "gps_gprmc") {
            return sensor_value_t(
                "GPRMC", _gps_gprmc(), "");
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

    float _zeroize(float x)
    {
      return boost::math::isnan(x) ? 0.0 : x;
    }

    int _nmea_checksum(const std::string &s)
    {
        if ((s.at(0) != '$'))
            return 0;

        boost::uint8_t sum = '\0';
        for (size_t i = 1; i < s.size(); i++)
            sum ^= static_cast<boost::uint8_t>(s.at(i));

        return sum;
    }

    double _deg_to_dm(double angle)
    {
        double fraction, integer;
        fraction = std::modf(angle, &integer);
        return std::floor(angle) * 100 + fraction * 60;
    }

    std::string _gps_gprmc(void)
    {
        struct tm tm;
        time_t intfixtime;

        boost::shared_lock<boost::shared_mutex> l(_d_mutex);

        tm.tm_mday = tm.tm_mon = tm.tm_year = 0;
        tm.tm_hour = tm.tm_min = tm.tm_sec = 0;

        if (boost::math::isnan(_gps_data.fix.time) == 0) {
            intfixtime = (time_t) _gps_data.fix.time;
            (void)gmtime_r(&intfixtime, &tm);
            tm.tm_mon++;
            tm.tm_year %= 100;
        }
        std::string string = str(boost::format(
           "$GPRMC,%02d%02d%02d,%c,%09.4f,%c,%010.4f,%c,%.4f,%.3f,%02d%02d%02d,,")
        % tm.tm_hour
        % tm.tm_min
        % tm.tm_sec
        % (_gps_data.status ? 'A' : 'V')
        % _zeroize(_deg_to_dm(std::fabs(_gps_data.fix.latitude)))
        % ((_gps_data.fix.latitude > 0) ? 'N' : 'S')
        % _zeroize(_deg_to_dm(std::fabs(_gps_data.fix.longitude)))
        % ((_gps_data.fix.longitude > 0) ? 'E' : 'W')
        % _zeroize(_gps_data.fix.speed * MPS_TO_KNOTS)
        % _zeroize(_gps_data.fix.track)
        % tm.tm_mday % tm.tm_mon % tm.tm_year);

        string.append(str(
            boost::format("*%02X") % _nmea_checksum(string)));

        return string;
    }

    std::string _gps_gpgga(void)
    {
        struct tm tm;
        time_t intfixtime;

        // currently not supported, make it blank
        float mag_var = NAN;

        boost::shared_lock<boost::shared_mutex> l(_d_mutex);

        intfixtime = (time_t) _gps_data.fix.time;
        (void) gmtime_r(&intfixtime, &tm);

        std::string string = str(boost::format(
            "$GPGGA,%02d%02d%02d,%09.4f,%c,%010.4f,%c,%d,%02d,")
            % tm.tm_hour
            % tm.tm_min
            % tm.tm_sec
            % _deg_to_dm(std::fabs(_gps_data.fix.latitude))
            % ((_gps_data.fix.latitude > 0) ? 'N' : 'S')
            % _deg_to_dm(std::fabs(_gps_data.fix.longitude))
            % ((_gps_data.fix.longitude > 0) ? 'E' : 'W')
            % _gps_data.status
            % _gps_data.satellites_used);

        if (boost::math::isnan(_gps_data.dop.hdop))
            string.append(",");
        else
            string.append(
                str(boost::format("%.2f,") % _gps_data.dop.hdop));

        if (boost::math::isnan(_gps_data.fix.altitude))
            string.append(",");
        else
            string.append(
                str(boost::format("%.2f,M,") % _gps_data.fix.altitude));

        if (boost::math::isnan(_gps_data.separation))
            string.append(",");
        else
            string.append(
                str(boost::format("%.3f,M,") % _gps_data.separation));

        if (boost::math::isnan(mag_var))
            string.append(",");
        else {
            string.append(
                str(boost::format("%3.2f,%s") % std::fabs(mag_var)
                % (mag_var > 0 ? "E" : "W")));
        }

        string.append(str(
            boost::format("*%02X") % _nmea_checksum(string)));

        return string;
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
