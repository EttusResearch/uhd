#ifndef INCLUDED_UHD_USRP_UBLOX_CONTROL_HPP
#define INCLUDED_UHD_USRP_UBLOX_CONTROL_HPP

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <uhd/config.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/types/sensors.hpp>

#include "e300_async_serial.hpp"

namespace uhd { namespace usrp { namespace gps {

namespace ublox { namespace ubx {

class control : public virtual uhd::gps_ctrl
{
public:
    typedef boost::shared_ptr<control> sptr;

    static sptr make(const std::string &node, const size_t baud_rate);

    virtual void configure_message_rate(
        const boost::uint16_t msg,
        const boost::uint8_t rate) = 0;

    virtual void configure_antenna(
        const boost::uint16_t flags,
        const boost::uint16_t pins) = 0;

    virtual void configure_pps(
        const boost::uint32_t interval,
        const boost::uint32_t length,
        const boost::int8_t status,
        const boost::uint8_t time_ref,
        const boost::uint8_t flags,
        const boost::int16_t antenna_delay,
        const boost::int16_t rf_group_delay,
        const boost::int32_t user_delay) = 0;

    virtual void configure_rates(
        boost::uint16_t meas_rate,
        boost::uint16_t nav_rate,
        boost::uint16_t time_ref) = 0;
};
}} // namespace ublox::ubx

}}} // namespace
#endif // INCLUDED_UHD_USRP_UBLOX_CONTROL_HPP
