//
// Copyright 2014 Ettus Research LLC
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

#include "e300_sensor_manager.hpp"

#include <boost/thread.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>

#include <cstring>
#include <uhd/exception.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/usrp/gps_ctrl.hpp>

namespace uhd { namespace usrp { namespace e300 {

class e300_sensor_proxy : public e300_sensor_manager
{
public:
    e300_sensor_proxy(
        uhd::transport::zero_copy_if::sptr xport) : _xport(xport)
    {
    }

    std::vector<std::string> get_sensors()
    {
        return boost::assign::list_of("temp")("gps_locked")("gps_time");
    }

    uhd::sensor_value_t get_sensor(const std::string &key)
    {
        if (key == "temp")
            return get_mb_temp();
        else if (key == "gps_locked")
            return get_gps_lock();
        else if (key == "gps_time")
            return get_gps_time();
        else
            throw uhd::lookup_error(
                str(boost::format("Invalid sensor %s requested.") % key));
    }

    uhd::sensor_value_t get_mb_temp(void)
    {
        boost::mutex::scoped_lock(_mutex);
        sensor_transaction_t transaction;
        transaction.which = uhd::htonx<boost::uint32_t>(ZYNQ_TEMP);
        {
            uhd::transport::managed_send_buffer::sptr buff
                = _xport->get_send_buff(1.0);
            if (not buff or buff->size() < sizeof(transaction)) {
                throw uhd::runtime_error("sensor proxy send timeout");
            }
            std::memcpy(
                buff->cast<void *>(),
                &transaction,
                sizeof(transaction));
            buff->commit(sizeof(transaction));
        }
        {
            uhd::transport::managed_recv_buffer::sptr buff
                = _xport->get_recv_buff(1.0);

            if (not buff or buff->size() < sizeof(transaction))
                throw uhd::runtime_error("sensor proxy recv timeout");

            std::memcpy(
                &transaction,
                buff->cast<const void *>(),
                sizeof(transaction));
        }
        UHD_ASSERT_THROW(uhd::ntohx<boost::uint32_t>(transaction.which) == ZYNQ_TEMP);
        // TODO: Use proper serialization here ...
        return sensor_value_t(
            "temp",
            e300_sensor_manager::unpack_float_from_uint32_t(
                uhd::ntohx(transaction.value)),
            "C");
    }

    uhd::sensor_value_t get_gps_time(void)
    {
        boost::mutex::scoped_lock(_mutex);
        sensor_transaction_t transaction;
        transaction.which = uhd::htonx<boost::uint32_t>(GPS_TIME);
        {
            uhd::transport::managed_send_buffer::sptr buff
                = _xport->get_send_buff(1.0);
            if (not buff or buff->size() < sizeof(transaction)) {
                throw uhd::runtime_error("sensor proxy send timeout");
            }
            std::memcpy(
                buff->cast<void *>(),
                &transaction,
                sizeof(transaction));
            buff->commit(sizeof(transaction));
        }
        {
            uhd::transport::managed_recv_buffer::sptr buff
                = _xport->get_recv_buff(1.0);

            if (not buff or buff->size() < sizeof(transaction))
                throw uhd::runtime_error("sensor proxy recv timeout");

            std::memcpy(
                &transaction,
                buff->cast<const void *>(),
                sizeof(transaction));
        }
        UHD_ASSERT_THROW(uhd::ntohx<boost::uint32_t>(transaction.which) == GPS_TIME);
        // TODO: Use proper serialization here ...
        return sensor_value_t("GPS epoch time", int(uhd::ntohx<boost::uint32_t>(transaction.value)), "seconds");
    }

    bool get_gps_found(void)
    {
        boost::mutex::scoped_lock(_mutex);
        sensor_transaction_t transaction;
        transaction.which = uhd::htonx<boost::uint32_t>(GPS_FOUND);
        {
            uhd::transport::managed_send_buffer::sptr buff
                = _xport->get_send_buff(1.0);
            if (not buff or buff->size() < sizeof(transaction)) {
                throw uhd::runtime_error("sensor proxy send timeout");
            }
            std::memcpy(
                buff->cast<void *>(),
                &transaction,
                sizeof(transaction));
            buff->commit(sizeof(transaction));
        }
        {
            uhd::transport::managed_recv_buffer::sptr buff
                = _xport->get_recv_buff(1.0);

            if (not buff or buff->size() < sizeof(transaction))
                throw uhd::runtime_error("sensor proxy recv timeout");

            std::memcpy(
                &transaction,
                buff->cast<const void *>(),
                sizeof(transaction));
        }
        UHD_ASSERT_THROW(uhd::ntohx<boost::uint32_t>(transaction.which) == GPS_FOUND);
        // TODO: Use proper serialization here ...
        return static_cast<bool>(uhd::ntohx(transaction.value));
    }

    uhd::sensor_value_t get_gps_lock(void)
    {
        boost::mutex::scoped_lock(_mutex);
        sensor_transaction_t transaction;
        transaction.which = uhd::htonx<boost::uint32_t>(GPS_LOCK);
        {
            uhd::transport::managed_send_buffer::sptr buff
                = _xport->get_send_buff(1.0);
            if (not buff or buff->size() < sizeof(transaction)) {
                throw uhd::runtime_error("sensor proxy send timeout");
            }
            std::memcpy(
                buff->cast<void *>(),
                &transaction,
                sizeof(transaction));
            buff->commit(sizeof(transaction));
        }
        {
            uhd::transport::managed_recv_buffer::sptr buff
                = _xport->get_recv_buff(1.0);

            if (not buff or buff->size() < sizeof(transaction))
                throw uhd::runtime_error("sensor proxy recv timeout");

            std::memcpy(
                &transaction,
                buff->cast<const void *>(),
                sizeof(transaction));
        }
        UHD_ASSERT_THROW(uhd::ntohx<boost::uint32_t>(transaction.which) == GPS_LOCK);
        // TODO: Use proper serialization here ...
        return sensor_value_t("GPS lock status", static_cast<bool>(uhd::ntohx(transaction.value)), "locked", "unlocked");
    }

private:
    uhd::transport::zero_copy_if::sptr _xport;
    boost::mutex                       _mutex;
};

}}} // namespace

using namespace uhd::usrp::e300;

e300_sensor_manager::sptr e300_sensor_manager::make_proxy(
    uhd::transport::zero_copy_if::sptr xport)
{
    return sptr(new e300_sensor_proxy(xport));
}

#ifdef E300_NATIVE
#include "e300_fifo_config.hpp"

namespace uhd { namespace usrp { namespace e300 {

static const std::string E300_TEMP_SYSFS = "iio:device0";

class e300_sensor_local : public e300_sensor_manager
{
public:
    e300_sensor_local(uhd::gps_ctrl::sptr gps_ctrl) : _gps_ctrl(gps_ctrl)
    {
    }

    std::vector<std::string> get_sensors()
    {
        return boost::assign::list_of("temp")("gps_locked")("gps_time");
    }

    uhd::sensor_value_t get_sensor(const std::string &key)
    {
        if (key == "temp")
            return get_mb_temp();
        else if (key == "gps_locked")
            return get_gps_lock();
        else if (key == "gps_time")
            return get_gps_time();
        else
            throw uhd::lookup_error(
                str(boost::format("Invalid sensor %s requested.") % key));
    }

    uhd::sensor_value_t get_mb_temp(void)
    {
        double scale = boost::lexical_cast<double>(
            e300_get_sysfs_attr(E300_TEMP_SYSFS, "in_temp0_scale"));
        unsigned long raw = boost::lexical_cast<unsigned long>(
            e300_get_sysfs_attr(E300_TEMP_SYSFS, "in_temp0_raw"));
        unsigned long offset = boost::lexical_cast<unsigned long>(
            e300_get_sysfs_attr(E300_TEMP_SYSFS, "in_temp0_offset"));
        return sensor_value_t("temp", (raw + offset) * scale / 1000, "C");
    }

    bool get_gps_found(void)
    {
        return _gps_ctrl->gps_detected();
    }

    uhd::sensor_value_t get_gps_lock(void)
    {
        return _gps_ctrl->get_sensor("gps_locked");
    }

    uhd::sensor_value_t get_gps_time(void)
    {
        return _gps_ctrl->get_sensor("gps_time");
    }

private:
    gps_ctrl::sptr _gps_ctrl;
};
}}}

using namespace uhd::usrp::e300;
e300_sensor_manager::sptr e300_sensor_manager::make_local(
    uhd::gps_ctrl::sptr gps_ctrl)
{
    return sptr(new e300_sensor_local(gps_ctrl));
}

#else
using namespace uhd::usrp::e300;
e300_sensor_manager::sptr e300_sensor_manager::make_local(
    uhd::gps_ctrl::sptr gps_ctrl)
{
    throw uhd::assertion_error("e300_sensor_manager::make_local() !E300_NATIVE");
}
#endif
