//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b300_gps_control.hpp"
#include "b300_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/ublox_msg_helper.hpp>
#include <uhdlib/usrp/gps_ctrl.hpp>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace {
constexpr uint32_t GPS_UART_CLKDIV  = 0x00;
constexpr uint32_t GPS_UART_TXLEVEL = 0x04;
constexpr uint32_t GPS_UART_RXLEVEL = 0x08;
constexpr uint32_t GPS_UART_TXCHAR  = 0x0C;
constexpr uint32_t GPS_UART_RXCHAR  = 0x10;
} // namespace

namespace uhd { namespace usrp { namespace b300 {

// This UART interface explicitly ignores the ublox messages that can also be coming from
// the UART interface of the chip. That is because this interface is only passed to the
// gps_control object which is only concerned with the NMEA messages. So, this allows for
// a cleaner implementation. The Ublox messages can still be read by a function that is
// only within the b300_gps_control_impl class.
struct b300_uart_iface : uart_iface
{
    b300_uart_iface(std::function<void(const byte_vector_t&)>&& write_bytes,
        std::function<byte_vector_t(size_t, size_t)>&& read_bytes)
        : _write_bytes(std::move(write_bytes)), _read_bytes(std::move(read_bytes))
    {
    }

    void write_uart(const std::string& buff) override
    {
        byte_vector_t bytes(buff.begin(), buff.end());
        _write_bytes(bytes);
    }

    std::string read_uart(double timeout) override
    {
        // Once an NMEA sentence has started, the remaining bytes are read with a
        // guaranteed minimum timeout. The caller may poll with timeout==0, but if
        // we used that timeout for every byte we would routinely return partial
        // sentences whenever the host caught up to the GPS transmit rate mid-line,
        // leaving the rest of the sentence in the FIFO to fragment the next read.
        // Note: for the B300 the timeout is expressed in milliseconds.
        size_t timeout_ms = static_cast<size_t>(std::llround(timeout * 1000.0));
        const size_t char_timeout =
            std::max(timeout_ms, static_cast<size_t>(std::llround(MIN_CHAR_TIMEOUT_MS)));
        byte_vector_t message{};
        bool started_gps_message = false;
        try {
            do {
                message = _read_bytes(1, timeout_ms);
                // Read until we get the start of an NMEA message.
                if (message[0] == '$') {
                    message.push_back(_read_bytes(1, char_timeout)[0]);
                    // Make sure we didn't get a random $ that wasn't a part of a proper
                    // NMEA message.
                    if (message[1] == 'G') {
                        started_gps_message = true;
                    }
                }
            } while (!started_gps_message);
            // Keep reading until the end of the NMEA message.
            while (message.back() != '\n') {
                message.push_back(_read_bytes(1, char_timeout)[0]);
            }
        } catch (const uhd::runtime_error&) {
            return "";
        }
        std::string uart_string(message.begin(), message.end());
        return uart_string;
    }

    std::function<void(const byte_vector_t&)> _write_bytes;
    std::function<byte_vector_t(size_t, size_t)> _read_bytes;

    // Minimum per-character timeout (in milliseconds) used once a sentence has
    // started, so a complete NMEA line is read atomically even when polling with
    // timeout==0.
    static constexpr double MIN_CHAR_TIMEOUT_MS = 100.0;
};

uart_iface::sptr b300_make_uart_iface(
    std::function<void(const byte_vector_t&)>&& write_bytes,
    std::function<byte_vector_t(size_t, size_t)>&& read_bytes)
{
    return uart_iface::sptr(
        new b300_uart_iface(std::move(write_bytes), std::move(read_bytes)));
}

class b300_gps_control_impl : public b300_gps_control
{
public:
    b300_gps_control_impl(
        write_fn_t&& poke32, read_fn_t&& peek32, power_on_fn_t&& power_on_fn)
        : _poke32(std::move(poke32))
        , _peek32(std::move(peek32))
        , _power_on_gps(std::move(power_on_fn))
        , _initialized(false)
    {
    }

    ~b300_gps_control_impl()
    {
        try {
            if (is_initialized()) {
                shutdown();
            }
        } catch (...) {
        }
    }

    void initialize() override
    {
        // Take GPS out of reset and enable antenna power.
        _power_on_gps(true);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        bool set_result = _set_values({
            {CFG_TP_DRSTR_TP1, {0x03}} // Set Drive Strength of TP1 to 12 mA
        });
        if (!set_result) {
            throw uhd::runtime_error("Error initializing GPS Chip!");
        }

        // clang-format off
        set_result = _set_values({
            {CFG_TP_TP1_ENA,          {0x01}}, // Enable the Time Pulse (TP1)
            {CFG_TP_PULSE_DEF,        {0x00}}, // Set Time Pulse to interpret as period
            {CFG_TP_PULSE_LENGTH_DEF, {0x01}}, // Set Time Pulse to interpret as length (us)
            {CFG_TP_PERIOD_TP1,       {0x00, 0x00, 0x00, 0x00}}, // Disable TP Period when there is no GPS lock
            {CFG_TP_LEN_TP1,          {0x00, 0x00, 0x00, 0x00}}, // Disable TP when there is no GPS lock
            {CFG_TP_POL_TP1,          {0x00}}, // Set TP1 polarity to falling edge
            {CFG_TP_TIMEGRID_TP1,     {0x01}}, // Set TP1 Time grid to use GPS time reference
            {CFG_TP_ALIGN_TO_TOW_TP1, {0x01}}, // Set Align TP1 to top of second
            {CFG_TP_USE_LOCKED_TP1,   {0x01}}, // Use locked parameters when possible for TP1
            {CFG_TP_POL_TP1,          {0x01}}, // Set TP1 polarity to rising edge
            {CFG_TP_PERIOD_LOCK_TP1,  {0x00, 0x0F, 0x42, 0x40}}, // Set TP1 period when locked to GNSS time (us)
            {CFG_TP_LEN_LOCK_TP1,     {0x00, 0x07, 0xA1, 0x20}}, // Set TP1 length when locked to GNSS time (us)
            {CFG_TP_POL_TP1,          {0x01}} // Set TP1 polarity to rising edge
        });
        // clang-format on
        if (!set_result) {
            throw uhd::runtime_error("Error initializing GPS Chip!");
        }

        // Only have GPS enabled by default. Explicitly disable other GNSS.
        // Enable all NMEA message types for detailed GPS logging and diagnostics.
        // clang-format off
        set_result = _set_values({
            {CFG_SIGNAL_GPS_ENA,           {0x01}},
            {CFG_SIGNAL_GPS_L1CA_ENA,      {0x01}},
            {CFG_SIGNAL_GAL_ENA,           {0x00}},
            {CFG_SIGNAL_BDS_ENA,           {0x00}},
            {CFG_SIGNAL_QZSS_ENA,          {0x00}},
            {CFG_MSGOUT_NMEA_ID_GSA_UART1, {0x01}},  // Enable GSA (Active satellites/DOP)
            {CFG_MSGOUT_NMEA_ID_GSV_UART1, {0x01}},  // Enable GSV (Satellite view)
            {CFG_MSGOUT_NMEA_ID_GLL_UART1, {0x01}},  // Enable GLL (Geographic position)
            {CFG_MSGOUT_NMEA_ID_VTG_UART1, {0x01}},  // Enable VTG (Track & speed)
            {CFG_MSGOUT_NMEA_ID_ZDA_UART1, {0x01}},  // Enable ZDA (Date & time)
        });
        // clang-format on
        if (!set_result) {
            throw uhd::runtime_error("Error initializing GPS Chip!");
        }

        // Create the GPS Control which will parse through the NMEA messages.
        _gps = gps_ctrl::make(
            b300_make_uart_iface(
                [this](const uhd::byte_vector_t& vec) { _write_bytes(vec); },
                [this](size_t num_bytes, size_t timeout_ms) {
                    return _read_bytes(num_bytes, timeout_ms);
                }),
            true);

        _initialized = true;
    }

    void shutdown() override
    {
        // Put GPS into reset and disable antenna power.
        _power_on_gps(false);
        _gps.reset();
        _initialized = false;
    }

    bool is_initialized() const override
    {
        return _initialized;
    }

    uhd::sensor_value_t get_sensor(std::string key) override
    {
        return _gps->get_sensor(std::move(key));
    }

private:
    // Set Values for the UBLOX GPS Chip
    bool _set_values(const uhd::dict<uint32_t, byte_vector_t>& settings)
    {
        _write_bytes(construct_valset_message(3, settings));
        bool result  = _get_message_acknowledged();
        size_t tries = 0;
        while (!result && tries < 3) {
            tries++;
            _write_bytes(construct_valset_message(3, settings));
            result = _get_message_acknowledged();
        }
        return result;
    }

    // Low-level function that writes to the FPGA registers used for the GPS UART
    // communication.
    void _write_bytes(const byte_vector_t& message)
    {
        std::lock_guard<std::mutex> lock(_write_mutex);
        for (const uint8_t byte : message) {
            auto start_time   = std::chrono::steady_clock::now();
            auto timeout      = std::chrono::milliseconds(1000);
            uint32_t tx_level = _peek32(GPS_UART_TXLEVEL);
            while (tx_level == 0
                   && (std::chrono::steady_clock::now() - start_time) < timeout) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                tx_level = _peek32(GPS_UART_TXLEVEL);
            }
            if (tx_level == 0) {
                throw uhd::runtime_error("Timeout waiting to write GPS data");
            }
            _poke32(GPS_UART_TXCHAR, byte);
        }
    }

    // Get Values for the UBLOX GPS Chip
    bool _get_values(uhd::dict<uint32_t, byte_vector_t>& settings)
    {
        _write_bytes(construct_valget_message(0, settings));
        byte_vector_t poll_return;
        bool result  = _get_poll_return_and_ack_messages(poll_return);
        size_t tries = 0;
        while (!result && tries < 3) {
            tries++;
            _write_bytes(construct_valget_message(0, settings));
            result = _get_poll_return_and_ack_messages(poll_return);
        }
        return result;
    }

    // Low-level function that reads from the FPGA registers used for the GPS UART
    // communication.
    byte_vector_t _read_bytes(size_t num_bytes, size_t timeout_ms = 100)
    {
        std::lock_guard<std::mutex> lock(_read_mutex);
        byte_vector_t return_vector(num_bytes);
        for (size_t i = 0; i < num_bytes; ++i) {
            auto start_time   = std::chrono::steady_clock::now();
            auto timeout      = std::chrono::milliseconds(timeout_ms);
            uint32_t rx_level = _peek32(GPS_UART_RXLEVEL);
            while (rx_level == 0
                   && (std::chrono::steady_clock::now() - start_time) < timeout) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                rx_level = _peek32(GPS_UART_RXLEVEL);
            }
            if (rx_level == 0) {
                throw uhd::runtime_error("Timeout waiting for GPS data");
            }
            return_vector[i] = static_cast<uint8_t>(_peek32(GPS_UART_RXCHAR) & 0xFF);
        }
        return return_vector;
    }

    // Waits for and retrieves both the poll return message and the ACK message for a
    // UBLOX poll request.
    bool _get_poll_return_and_ack_messages(byte_vector_t& return_message)
    {
        return_message  = _read_ublox_message();
        auto start_time = std::chrono::steady_clock::now();
        // ACK should happen within a second
        auto timeout = std::chrono::milliseconds(1000);
        // Sometimes we don't see the poll return, but see the ACK, so be looking for that
        // too and alert the user that the return didn't happen.
        while ((return_message[2] != 0x06 || return_message[3] != 0x8b)
               && (std::chrono::steady_clock::now() - start_time) < timeout) {
            try {
                return_message = _read_ublox_message();
            } catch (const uhd::runtime_error&) {
                return false;
            }
        }
        if ((return_message[2] != 0x06 || return_message[3] != 0x8b)) {
            return false;
        }
        return _get_message_acknowledged();
    }

    // Waits for and retrieves the ACK message for a UBLOX command.
    bool _get_message_acknowledged()
    {
        byte_vector_t return_message = _read_ublox_message();
        auto start_time              = std::chrono::steady_clock::now();
        // ACK should happen within a second
        auto timeout = std::chrono::milliseconds(1000);
        while (return_message[2] != 0x05
               && (std::chrono::steady_clock::now() - start_time) < timeout) {
            try {
                return_message = _read_ublox_message();
            } catch (const uhd::runtime_error&) {
                return false;
            }
        }
        if (return_message[2] != 0x05 || return_message[3] == 0x00) {
            UHD_LOG_ERROR("B300_GPS", "Poll Request Not Acknowledged!");
            return false;
        } else if (return_message[3] == 0x01) {
            return true;
        } else {
            UHD_LOG_ERROR("B300_GPS", "Invalid UBLOX ACK ID!");
            return false;
        }
    }

    // Parses the UART output, looking specifically for UBLOX messages. This function
    // ignores and dumps NMEA messages, which is ok since those are constantly being
    // output, and will be stale by the time the gps_ctrl reads them.
    byte_vector_t _read_ublox_message()
    {
        // The B310 GPS Chip can return either UBLOX messages (noted by the first byte of
        // 0xb5) or NMEA messages (noted by the first byte of 0x24 or ASCII '$').
        byte_vector_t message = _read_bytes(1);
        while (message[0] != 0xb5) {
            message = _read_bytes(1);
        }
        // Get the rest of the standard first 6 bytes of the message from the GPS Chip.
        byte_vector_t ublox_header = _read_bytes(5);
        message.insert(message.end(), ublox_header.begin(), ublox_header.end());
        if (message[1] != 0x62) {
            // All UBLOX headers are 0xb5 0x62, so if we saw a 0xb5 followed by anything
            // else, then something went wrong. NMEA messages should fall in the range of
            // 0x20 to 0x7E so we shouldn't accidentally pick up the middle of a NMEA
            // message with 0xb5.
            throw uhd::runtime_error("Invalid UBLOX header returned from GPS Chip.");
        }
        // Calculate the remaining number of bytes in the message (+2 for the checksum
        // bits).
        uint16_t payload_length            = (message[5] << 8) | message[4];
        byte_vector_t payload_and_checksum = _read_bytes(payload_length + 2);
        message.insert(
            message.end(), payload_and_checksum.begin(), payload_and_checksum.end());
        return message;
    }

    std::mutex _read_mutex;
    std::mutex _write_mutex;
    write_fn_t _poke32;
    read_fn_t _peek32;
    power_on_fn_t _power_on_gps;
    uhd::gps_ctrl::sptr _gps;
    bool _initialized;
};

b300_gps_control::sptr b300_gps_control::make(
    write_fn_t&& poke32, read_fn_t&& peek32, power_on_fn_t&& power_on_fn)
{
    return std::make_shared<b300_gps_control_impl>(
        std::move(poke32), std::move(peek32), std::move(power_on_fn));
}
}}} // namespace uhd::usrp::b300
