#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/assign/list_of.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"

#include <iostream>


#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>

#include "e300_ublox_control.hpp"

#ifdef E300_NATIVE
#include "e300_ublox_control_impl.hpp"


namespace uhd { namespace usrp { namespace gps {

namespace ublox { namespace ubx {

control_impl::control_impl(const std::string &node, const size_t baud_rate)
{
    _decode_init();
    _serial = boost::make_shared<async_serial>(node, baud_rate);
    _serial->set_read_callback(boost::bind(&control_impl::_rx_callback, this, _1, _2));

    _detect();

    configure_message_rate(MSG_GLL, 0);
    configure_message_rate(MSG_GSV, 0);
    configure_message_rate(MSG_GGA, 0);
    configure_message_rate(MSG_GSA, 0);
    configure_message_rate(MSG_RMC, 0);
    configure_message_rate(MSG_VTG, 0);
    configure_message_rate(MSG_NAV_TIMEUTC, 1);
    configure_message_rate(MSG_NAV_SOL, 1);

    configure_antenna(0x001b, 0x8251);

    configure_pps(0xf4240, 0x3d090, 1, 0 /* utc */, 1, 0, 0, 0);

    _sensors = boost::assign::list_of("gps_locked")("gps_time");
}

bool control_impl::gps_detected(void)
{
    return _detected;
}

void control_impl::_detect(void)
{
    _send_message(MSG_MON_VER, NULL, 0);
}

std::vector<std::string> control_impl::get_sensors(void)
{
    return _sensors;
}

uhd::sensor_value_t control_impl::get_sensor(std::string key)
{
    if (key == "gps_time") {
        return sensor_value_t("GPS epoch time", int(_get_epoch_time()), "seconds");
    } else if (key == "gps_locked") {
        bool lock;
        _locked.wait_and_see(lock);
        return sensor_value_t("GPS lock status", lock, "locked", "unlocked");
    } else
        throw uhd::key_error(str(boost::format("sensor %s unknown.") % key));
}

std::time_t control_impl::_get_epoch_time(void)
{
    boost::posix_time::ptime ptime;
    _ptime.wait_and_see(ptime);
    return (ptime - boost::posix_time::from_time_t(0)).total_seconds();
}

control_impl::~control_impl(void)
{
    // turn it all off again
    configure_antenna(0x001a, 0x8251);
    configure_pps(0xf4240, 0x3d090, 1, 1, 0, 0, 0, 0);
}

void control_impl::_decode_init(void)
{
    _decode_state = DECODE_SYNC1;
    _rx_ck_a = 0;
    _rx_ck_b = 0;
    _rx_payload_length = 0;
    _rx_payload_index  = 0;
}

void control_impl::_add_byte_to_checksum(const boost::uint8_t b)
{
    _rx_ck_a = _rx_ck_a + b;
    _rx_ck_b = _rx_ck_b + _rx_ck_a;
}

void control_impl::_calc_checksum(
    const boost::uint8_t *buffer,
    const boost::uint16_t length,
    checksum_t &checksum)
{
    for (size_t i = 0; i < length; i++)
    {
        checksum.ck_a = checksum.ck_a + buffer[i];
        checksum.ck_b = checksum.ck_b + checksum.ck_a;
    }
}

void control_impl::configure_rates(
    boost::uint16_t meas_rate,
    boost::uint16_t nav_rate,
    boost::uint16_t time_ref)
{
    payload_tx_cfg_rate_t cfg_rate;
    cfg_rate.meas_rate = uhd::htowx<boost::uint16_t>(meas_rate);
    cfg_rate.nav_rate = uhd::htowx<boost::uint16_t>(nav_rate);
    cfg_rate.time_ref = uhd::htowx<boost::uint16_t>(time_ref);

    _send_message(
        MSG_CFG_RATE,
        reinterpret_cast<const uint8_t*>(&cfg_rate),
        sizeof(cfg_rate));

    _wait_for_ack(MSG_CFG_RATE, 1.0);
}

void control_impl::configure_message_rate(
    const boost::uint16_t msg,
    const uint8_t rate)
{
    payload_tx_cfg_msg_t cfg_msg;
    cfg_msg.msg  = uhd::htowx<boost::uint16_t>(msg);
    cfg_msg.rate[0] = 0;//rate;
    cfg_msg.rate[1] = rate;
    cfg_msg.rate[2] = 0;//rate;
    cfg_msg.rate[3] = 0;//rate;
    cfg_msg.rate[4] = 0;//rate;
    cfg_msg.rate[5] = 0;//rate;
    _send_message(
        MSG_CFG_MSG,
        reinterpret_cast<const uint8_t*>(&cfg_msg),
        sizeof(cfg_msg));

    _wait_for_ack(MSG_CFG_MSG, 1.0);
}

void control_impl::configure_antenna(
    const boost::uint16_t flags,
    const boost::uint16_t pins)
{
    payload_tx_cfg_ant_t cfg_ant;
    cfg_ant.pins = uhd::htowx<boost::uint16_t>(pins);
    cfg_ant.flags = uhd::htowx<boost::uint16_t>(flags);
    _send_message(
        MSG_CFG_ANT,
        reinterpret_cast<const uint8_t*>(&cfg_ant),
        sizeof(cfg_ant));
    if (_wait_for_ack(MSG_CFG_ANT, 1.0) < 0) {
        throw uhd::runtime_error("Didn't get an ACK for antenna configuration.");
    }

}

void control_impl::configure_pps(
    const boost::uint32_t interval,
    const boost::uint32_t length,
    const boost::int8_t status,
    const boost::uint8_t time_ref,
    const boost::uint8_t flags,
    const boost::int16_t antenna_delay,
    const boost::int16_t rf_group_delay,
    const boost::int32_t user_delay)
{
    payload_tx_cfg_tp_t cfg_tp;
    cfg_tp.interval = uhd::htowx<boost::uint32_t>(interval);
    cfg_tp.length = uhd::htowx<boost::uint32_t>(length);
    cfg_tp.status = status;
    cfg_tp.time_ref = time_ref;
    cfg_tp.flags = flags;
    cfg_tp.antenna_delay = uhd::htowx<boost::int16_t>(antenna_delay);
    cfg_tp.rf_group_delay = uhd::htowx<boost::int16_t>(rf_group_delay);
    cfg_tp.user_delay = uhd::htowx<boost::int32_t>(user_delay);
    _send_message(
        MSG_CFG_TP,
        reinterpret_cast<const uint8_t*>(&cfg_tp),
        sizeof(cfg_tp));
    if (_wait_for_ack(MSG_CFG_TP, 1.0) < 0) {
        throw uhd::runtime_error("Didn't get an ACK for PPS configuration.");
    }
}


void control_impl::_rx_callback(const char *data, unsigned int len)
{
    //std::cout << "IN RX CALLBACK" << std::flush << std::endl;
    std::vector<char> v(data, data+len);
    BOOST_FOREACH(const char &c, v)
    {
        _parse_char(c);
    }
}

void control_impl::_parse_char(const boost::uint8_t b)
{
    int ret = 0;

    switch (_decode_state) {

    // we're expecting the first sync byte
    case DECODE_SYNC1:
        if (b == SYNC1) { // sync1 found goto next step
            _decode_state = DECODE_SYNC2;
        } // else stay around
        break;

    // we're expecting the second sync byte
    case DECODE_SYNC2:
        if (b == SYNC2) { // sync2 found goto next step
            _decode_state = DECODE_CLASS;
        } else {
            // failed, reset
            _decode_init();
        }
        break;

    // we're expecting the class byte
    case DECODE_CLASS:
        _add_byte_to_checksum(b);
        _rx_msg = b;
        _decode_state = DECODE_ID;
        break;

    // we're expecting the id byte
    case DECODE_ID:
        _add_byte_to_checksum(b);
        _rx_msg |= (b << 8);
        _decode_state = DECODE_LENGTH1;
        break;

    // we're expecting the first length byte
    case DECODE_LENGTH1:
        _add_byte_to_checksum(b);
        _rx_payload_length = b;
        _decode_state = DECODE_LENGTH2;
        break;

    // we're expecting the second length byte
    case DECODE_LENGTH2:
        _add_byte_to_checksum(b);
        _rx_payload_length |= (b << 8);
        if(_payload_rx_init()) {
            _decode_init(); // we failed, give up for this one
        } else {
            _decode_state = _rx_payload_length ?
                DECODE_PAYLOAD : DECODE_CHKSUM1;
        }
        break;

    // we're expecting payload
    case DECODE_PAYLOAD:
        _add_byte_to_checksum(b);
        switch(_rx_msg) {
        default:
            ret = _payload_rx_add(b);
            break;
        };
        if (ret < 0) {
            // we couldn't deal with the payload, discard the whole thing
            _decode_init();
        } else if (ret > 0) {
            // payload was complete, let's check the checksum;
            _decode_state = DECODE_CHKSUM1;
        } else {
            // more payload expected, don't move
        }
        ret = 0;
        break;

    case DECODE_CHKSUM1:
        if (_rx_ck_a != b) {
            // checksum didn't match, barf
            std::cout << boost::format("Failed checksum byte1 %lx != %lx")
                % int(_rx_ck_a) % int(b) << std::endl;
            _decode_init();
        } else {
            _decode_state = DECODE_CHKSUM2;
        }
        break;

    case DECODE_CHKSUM2:
        if (_rx_ck_b != b) {
            // checksum didn't match, barf
            std::cout << boost::format("Failed checksum byte2 %lx != %lx")
                % int(_rx_ck_b) % int(b) << std::endl;

        } else {
            ret = _payload_rx_done(); // payload done
        }
        _decode_init();
        break;

    default:
        break;
    };
}

int control_impl::_payload_rx_init(void)
{
    int ret = 0;

    _rx_state = RXMSG_HANDLE; // by default handle
    switch(_rx_msg) {

    case MSG_NAV_SOL:
        if (not (_rx_payload_length == sizeof(payload_rx_nav_sol_t)))
            _rx_state = RXMSG_ERROR_LENGTH;
        break;

    case MSG_NAV_TIMEUTC:
        if (not (_rx_payload_length == sizeof(payload_rx_nav_timeutc_t)))
            _rx_state = RXMSG_ERROR_LENGTH;
        break;

    case MSG_MON_VER:
        break; // always take this one

    case MSG_ACK_ACK:
        if (not (_rx_payload_length == sizeof(payload_rx_ack_ack_t)))
            _rx_state = RXMSG_ERROR_LENGTH;
        break;

    case MSG_ACK_NAK:
        if (not (_rx_payload_length == sizeof(payload_rx_ack_nak_t)))
            _rx_state = RXMSG_ERROR_LENGTH;
        break;

    default:
        _rx_state = RXMSG_DISABLE;
        break;
    };

    switch (_rx_state) {
    case RXMSG_HANDLE: // handle message
    case RXMSG_IGNORE: // ignore message but don't report error
        ret = 0;
        break;
    case RXMSG_DISABLE: // ignore message but don't report error
    case RXMSG_ERROR_LENGTH: // the length doesn't match
        ret = -1;
        break;
    default: // invalid, error
        ret = -1;
        break;
    };

    return ret;
}

int control_impl::_payload_rx_add(const boost::uint8_t b)
{
    int ret = 0;
    _buf.raw[_rx_payload_index] = b;
    if (++_rx_payload_index >= _rx_payload_length)
        ret = 1;
    return ret;
}

int control_impl::_payload_rx_done(void)
{
    int ret = 0;
    if (_rx_state != RXMSG_HANDLE) {
        return 0;
    }

    switch (_rx_msg) {
    case MSG_MON_VER:
        _detected = true;
        break;

    case MSG_MON_HW:
        std::cout << "MON-HW" << std::endl;
        break;

    case MSG_ACK_ACK:
        if ((_ack_state == ACK_WAITING) and (_buf.payload_rx_ack_ack.msg == _ack_waiting_msg))
            _ack_state = ACK_GOT_ACK;
        break;

    case MSG_ACK_NAK:
        if ((_ack_state == ACK_WAITING) and (_buf.payload_rx_ack_nak.msg == _ack_waiting_msg))
            _ack_state = ACK_GOT_NAK;

        break;

    case MSG_CFG_ANT:
        break;

    case MSG_NAV_TIMEUTC:
        _ptime.update(boost::posix_time::ptime(
            boost::gregorian::date(
                boost::gregorian::greg_year(uhd::wtohx<boost::uint16_t>(
                    _buf.payload_rx_nav_timeutc.year)),
                boost::gregorian::greg_month(_buf.payload_rx_nav_timeutc.month),
                boost::gregorian::greg_day(_buf.payload_rx_nav_timeutc.day)),
            (boost::posix_time::hours(_buf.payload_rx_nav_timeutc.hour)
            + boost::posix_time::minutes(_buf.payload_rx_nav_timeutc.min)
            + boost::posix_time::seconds(_buf.payload_rx_nav_timeutc.sec))));
        break;

    case MSG_NAV_SOL:
        _locked.update(_buf.payload_rx_nav_sol.gps_fix > 0);
        break;

    default:
        std::cout << boost::format("Got unknown message %lx , with good checksum [") % int(_rx_msg);
        for(size_t i = 0; i < _rx_payload_length; i++)
            std::cout << boost::format("%lx, ") % int(_buf.raw[i]);
        std::cout << "]"<< std::endl;
        break;
    };
    return ret;
}

void control_impl::_send_message(
    const boost::uint16_t msg,
    const boost::uint8_t *payload,
    const boost::uint16_t len)
{
    header_t header = {SYNC1, SYNC2, msg, len};
    checksum_t checksum = {0, 0};

    // calculate checksums, first header without sync
    // then payload
    _calc_checksum(
        reinterpret_cast<boost::uint8_t*>(&header) + 2,
        sizeof(header) - 2, checksum);
    if (payload)
        _calc_checksum(payload, len, checksum);

    _serial->write(
        reinterpret_cast<const char*>(&header),
        sizeof(header));

    if (payload)
        _serial->write((const char *) payload, len);

    _serial->write(
        reinterpret_cast<const char*>(&checksum),
        sizeof(checksum));
}

int control_impl::_wait_for_ack(
    const boost::uint16_t msg,
    const double timeout)
{
    int ret = -1;

    _ack_state = ACK_WAITING;
    _ack_waiting_msg = msg;

    boost::system_time timeout_time =
        boost::get_system_time() +
        boost::posix_time::milliseconds(timeout * 1000.0);

    do {
        if(_ack_state == ACK_GOT_ACK)
            return 0;
        else if (_ack_state == ACK_GOT_NAK) {
            return -1;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
    } while (boost::get_system_time() < timeout_time);

    // we get here ... it's a timeout
    _ack_state = ACK_IDLE;
    return ret;
}


}} // namespace ublox::ubx
}}} // namespace

using namespace uhd::usrp::gps::ublox::ubx;

control::sptr control::make(const std::string &node, const size_t baud_rate)
{
    return control::sptr(new control_impl(node, baud_rate));
}
#else
using namespace uhd::usrp::gps::ublox::ubx;

control::sptr control::make(const std::string &node, const size_t baud_rate)
{
    throw uhd::assertion_error("control::sptr::make: !E300_NATIVE");
}
#endif // E300_NATIVE
