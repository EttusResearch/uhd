#ifndef INCLUDED_UHD_USRP_UBLOX_CONTROL_IMPL_HPP
#define INCLUDED_UHD_USRP_UBLOX_CONTROL_IMPL_HPP

#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <uhd/config.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/types/sensors.hpp>

#include "e300_async_serial.hpp"

namespace uhd { namespace usrp { namespace gps {

namespace ublox { namespace ubx {
// ublox binary sync words
static const boost::uint8_t SYNC1 = 0xB5;
static const boost::uint8_t SYNC2 = 0x62;

// message classes
static const boost::uint8_t CLASS_NAV  = 0x01;
static const boost::uint8_t CLASS_ACK  = 0x05;
static const boost::uint8_t CLASS_CFG  = 0x06;
static const boost::uint8_t CLASS_MON  = 0x0a;
static const boost::uint8_t CLASS_NMEA = 0xf0;

// Message IDs
static const boost::uint8_t ID_NAV_POSLLH  = 0x02;
static const boost::uint8_t ID_NAV_SOL     = 0x06;
static const boost::uint8_t ID_NAV_PVT     = 0x07;
static const boost::uint8_t ID_NAV_VELNED  = 0x12;
static const boost::uint8_t ID_NAV_TIMEUTC = 0x21;
static const boost::uint8_t ID_NAV_SVINFO  = 0x30;
static const boost::uint8_t ID_ACK_NAK     = 0x00;
static const boost::uint8_t ID_ACK_ACK     = 0x01;
static const boost::uint8_t ID_CFG_PRT     = 0x00;
static const boost::uint8_t ID_CFG_ANT     = 0x13;
static const boost::uint8_t ID_CFG_TP      = 0x07;
static const boost::uint8_t ID_CFG_MSG     = 0x01;
static const boost::uint8_t ID_CFG_RATE    = 0x08;
static const boost::uint8_t ID_CFG_NAV5    = 0x24;
static const boost::uint8_t ID_MON_VER     = 0x04;
static const boost::uint8_t ID_MON_HW      = 0x09;
static const boost::uint8_t ID_GGA         = 0x00;
static const boost::uint8_t ID_GLL         = 0x01;
static const boost::uint8_t ID_GSA         = 0x02;
static const boost::uint8_t ID_GSV         = 0x03;
static const boost::uint8_t ID_RMC         = 0x04;
static const boost::uint8_t ID_VTG         = 0x05;
static const boost::uint8_t ID_GST         = 0x07;

// Message Classes & IDs //
static const boost::uint16_t MSG_NAV_POSLLH
    = CLASS_NAV | (ID_NAV_POSLLH << 8);
static const boost::uint16_t MSG_NAV_SOL
    = CLASS_NAV | (ID_NAV_SOL << 8);
static const boost::uint16_t MSG_NAV_PVT
    = CLASS_NAV | (ID_NAV_PVT << 8);
static const boost::uint16_t MSG_NAV_VELNED
    = CLASS_NAV | (ID_NAV_VELNED << 8);
static const boost::uint16_t MSG_NAV_TIMEUTC
    = CLASS_NAV | (ID_NAV_TIMEUTC << 8);
static const boost::uint16_t MSG_NAV_SVINFO
    = CLASS_NAV | (ID_NAV_SVINFO << 8);
static const boost::uint16_t MSG_ACK_NAK
    = CLASS_ACK | (ID_ACK_NAK << 8);
static const boost::uint16_t MSG_ACK_ACK
    = CLASS_ACK | (ID_ACK_ACK << 8);
static const boost::uint16_t MSG_CFG_PRT
    = CLASS_CFG | (ID_CFG_PRT << 8);
static const boost::uint16_t MSG_CFG_ANT
    = CLASS_CFG | (ID_CFG_ANT << 8);
static const boost::uint16_t MSG_CFG_TP
    = CLASS_CFG | (ID_CFG_TP << 8);
static const boost::uint16_t MSG_CFG_MSG
    = CLASS_CFG | (ID_CFG_MSG << 8);
static const boost::uint16_t MSG_CFG_RATE
    = CLASS_CFG | (ID_CFG_RATE << 8);
static const boost::uint16_t MSG_CFG_NAV5
    = CLASS_CFG | (ID_CFG_NAV5 << 8);
static const boost::uint16_t MSG_MON_HW
    = CLASS_MON | (ID_MON_HW << 8);
static const boost::uint16_t MSG_MON_VER
    = CLASS_MON | (ID_MON_VER << 8);

// NMEA ones
static const boost::uint16_t MSG_GGA
    = CLASS_NMEA | (ID_GGA << 8);
static const boost::uint16_t MSG_GLL
    = CLASS_NMEA | (ID_GLL << 8);
static const boost::uint16_t MSG_GSA
    = CLASS_NMEA | (ID_GSA << 8);
static const boost::uint16_t MSG_GSV
    = CLASS_NMEA | (ID_GSV << 8);
static const boost::uint16_t MSG_RMC
    = CLASS_NMEA | (ID_RMC << 8);
static const boost::uint16_t MSG_VTG
    = CLASS_NMEA | (ID_VTG << 8);

// header
struct header_t
{
    boost::uint8_t sync1;
    boost::uint8_t sync2;
    boost::uint16_t msg;
    boost::uint16_t length;
};

// checksum
struct checksum_t
{
    boost::uint8_t ck_a;
    boost::uint8_t ck_b;
};

// rx rx mon-hw (ubx6)
struct payload_rx_mon_hw_t
{
    boost::uint32_t pin_sel;
    boost::uint32_t pin_bank;
    boost::uint32_t pin_dir;
    boost::uint32_t pin_val;
    boost::uint16_t noise_per_ms;
    boost::uint16_t agc_cnt;
    boost::uint8_t  a_status;
    boost::uint8_t  a_power;
    boost::uint8_t  flags;
    boost::uint8_t  reserved1;
    boost::uint32_t used_mask;
    boost::uint8_t  vp[25];
    boost::uint8_t  jam_ind;
    boost::uint16_t reserved3;
    boost::uint32_t pin_irq;
    boost::uint32_t pullh;
    boost::uint32_t pulll;
};

// rx mon-ver
struct payload_rx_mon_ver_part1_t
{
    char sw_version[30];
    char hw_version[10];
};

struct payload_rx_mon_ver_part2_t
{
    boost::uint8_t extension[30];
};

// rx ack-ack
typedef union {
    boost::uint16_t msg;
    struct {
        boost::uint8_t cls_id;
        boost::uint8_t msg_id;
    };
} payload_rx_ack_ack_t;

// rx ack-nak
typedef union {
    boost::uint16_t msg;
    struct {
        boost::uint8_t cls_id;
        boost::uint8_t msg_id;
    };
} payload_rx_ack_nak_t;

// tx cfg-prt (uart)
struct payload_tx_cfg_prt_t
{
    boost::uint8_t  port_id;
    boost::uint8_t  reserved0;
    boost::uint16_t tx_ready;
    boost::uint32_t mode;
    boost::uint32_t baud_rate;
    boost::uint16_t in_proto_mask;
    boost::uint16_t out_proto_mask;
    boost::uint16_t flags;
    boost::uint16_t reserved5;
};

// tx cfg-rate
struct payload_tx_cfg_rate_t
{
    boost::uint16_t meas_rate;
    boost::uint16_t nav_rate;
    boost::uint16_t time_ref;
};

// tx cfg-msg
struct payload_tx_cfg_msg_t
{
    boost::uint16_t msg;
    boost::uint8_t rate[6];
};


// tx cfg-ant
struct payload_tx_cfg_ant_t
{
    boost::uint16_t flags;
    boost::uint16_t pins;
};

// tx cfg-tp
struct payload_tx_cfg_tp_t
{
    boost::uint32_t interval;
    boost::uint32_t length;
    boost::int8_t status;
    boost::uint8_t time_ref;
    boost::uint8_t flags;
    boost::uint8_t reserved1;
    boost::int16_t antenna_delay;
    boost::int16_t rf_group_delay;
    boost::int32_t user_delay;
};

struct payload_rx_nav_sol_t
{
    boost::uint32_t i_tow;
    boost::int32_t f_tow;
    boost::int16_t week;
    boost::uint8_t gps_fix;
    boost::uint8_t flags;
    boost::int32_t ecef_x;
    boost::int32_t ecef_y;
    boost::int32_t ecef_z;
    boost::uint32_t p_acc;
    boost::int32_t ecef_vx;
    boost::int32_t ecef_vy;
    boost::int32_t ecef_vz;
    boost::uint32_t s_acc;
    boost::uint16_t p_dop;
    boost::uint8_t reserved1;
    boost::uint8_t num_sv;
    boost::uint32_t reserved2;
};

struct payload_rx_nav_timeutc_t
{
    boost::uint32_t i_tow;
    boost::uint32_t t_acc;
    boost::int32_t nano;
    boost::uint16_t year;
    boost::uint8_t month;
    boost::uint8_t day;
    boost::uint8_t hour;
    boost::uint8_t min;
    boost::uint8_t sec;
    boost::uint8_t valid;
};

typedef union {
    payload_rx_mon_hw_t        payload_rx_mon_hw;

    payload_rx_mon_ver_part1_t payload_rx_mon_ver_part1;
    payload_rx_mon_ver_part2_t payload_rx_mon_ver_part2;

    payload_rx_ack_ack_t       payload_rx_ack_ack;
    payload_rx_ack_nak_t       payload_rx_ack_nak;

    payload_tx_cfg_prt_t       payload_tx_cfg_prt;
    payload_tx_cfg_ant_t       payload_tx_cfg_ant;
    payload_tx_cfg_rate_t      payload_tx_cfg_rate;

    payload_tx_cfg_msg_t       payload_tx_cfg_msg;

    payload_rx_nav_timeutc_t   payload_rx_nav_timeutc;
    payload_rx_nav_sol_t   payload_rx_nav_sol;
    boost::uint8_t             raw[];
} buf_t;


template <typename T>
class sensor_entry
{
public:
    sensor_entry() : _seen(false)
    {
    }

    void update(const T &val)
    {
        boost::mutex::scoped_lock l(_mutex);
        _value = val;
        _seen = false;
        l.unlock();
        _cond.notify_one();
    }

    bool seen() const
    {
        boost::mutex::scoped_lock l(_mutex);
        return _seen;
    }

    bool try_and_see(T &val)
    {
        boost::mutex::scoped_lock l(_mutex);
        if (_seen)
            return false;

        val = _value;
        _seen = true;
        return true;
    }

    void wait_and_see(T &val)
    {
        boost::mutex::scoped_lock l(_mutex);
        while(_seen)
        {
            _cond.wait(l);
            //std::cout << "Already seen ... " << std::endl;
        }
        val = _value;
        _seen = true;
    }

private: // members
    T                         _value;
    boost::mutex              _mutex;
    boost::condition_variable _cond;
    bool                _seen;
};

class control_impl : public control
{
public:
    control_impl(const std::string &node, const size_t baud_rate);

    virtual ~control_impl(void);

    void configure_message_rate(
        const boost::uint16_t msg,
        const boost::uint8_t rate);

    void configure_antenna(
        const boost::uint16_t flags,
        const boost::uint16_t pins);

    void configure_pps(
        const boost::uint32_t interval,
        const boost::uint32_t length,
        const boost::int8_t status,
        const boost::uint8_t time_ref,
        const boost::uint8_t flags,
        const boost::int16_t antenna_delay,
        const boost::int16_t rf_group_delay,
        const boost::int32_t user_delay);

    void configure_rates(
        boost::uint16_t meas_rate,
        boost::uint16_t nav_rate,
        boost::uint16_t time_ref);

    // gps_ctrl interface
    bool gps_detected(void);
    std::vector<std::string> get_sensors(void);
    uhd::sensor_value_t get_sensor(std::string key);

private: // types
    enum decoder_state_t {
        DECODE_SYNC1 = 0,
        DECODE_SYNC2,
        DECODE_CLASS,
        DECODE_ID,
        DECODE_LENGTH1,
        DECODE_LENGTH2,
        DECODE_PAYLOAD,
        DECODE_CHKSUM1,
        DECODE_CHKSUM2,
    };

    enum rxmsg_state_t {
        RXMSG_IGNORE = 0,
        RXMSG_HANDLE,
        RXMSG_DISABLE,
        RXMSG_ERROR_LENGTH
    };

    enum ack_state_t {
        ACK_IDLE = 0,
        ACK_WAITING,
        ACK_GOT_ACK,
        ACK_GOT_NAK
    };

private: // methods
    std::time_t _get_epoch_time(void);

    void _decode_init(void);

    void _add_byte_to_checksum(const boost::uint8_t b);

    void _detect(void);

    void _send_message(
        const boost::uint16_t msg,
        const boost::uint8_t *payload,
        const boost::uint16_t len);

    int _wait_for_ack(
        const boost::uint16_t msg,
        const double timeout);

    void _calc_checksum(
        const boost::uint8_t *buffer,
        const boost::uint16_t length,
        checksum_t &checksum);

    void _rx_callback(const char *data, unsigned len);

    void _parse_char(const boost::uint8_t b);

    int _payload_rx_init(void);

    int _payload_rx_add(const boost::uint8_t b);

    int _payload_rx_done(void);

private: // members
    // gps_ctrl stuff
    bool                                   _detected;
    std::vector<std::string>               _sensors;

    sensor_entry<bool>                     _locked;
    sensor_entry<boost::posix_time::ptime> _ptime;

    // decoder state
    decoder_state_t                        _decode_state;
    rxmsg_state_t                          _rxmsg_state;

    ack_state_t                            _ack_state;
    boost::uint16_t                        _ack_waiting_msg;

    boost::uint8_t                         _rx_ck_a;
    boost::uint8_t                         _rx_ck_b;

    boost::uint16_t                        _rx_payload_length;
    size_t                                 _rx_payload_index;
    boost::uint16_t                        _rx_msg;

    rxmsg_state_t                          _rx_state;

    boost::shared_ptr<async_serial>        _serial;

    // this has to be at the end of the
    // class to be valid C++
    buf_t                                  _buf;
};

}} // namespace ublox::ubx

}}} // namespace
#endif // INCLUDED_UHD_USRP_UBLOX_CONTROL_IMPL_HPP
