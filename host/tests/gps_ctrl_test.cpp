//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/serial.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/log_add.hpp>
#include <uhdlib/usrp/gps_ctrl.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace {

const std::string GGA_MULTI =
    "$GNGGA,125740.00,4852.60000,S,12323.60000,W,2,12,1.28,237.1,M,25.1,M,,*48";
const std::string GGA_GPS =
    "$GPGGA,125740.00,4852.60000,S,12323.60000,W,2,12,1.28,237.1,M,25.1,M,,*56";

const std::string GSV_GPS =
    "$GPGSV,3,1,12,02,23,273,24,10,70,166,46,13,04,014,25,16,23,211,40,1*67";
const std::string GSV_GLONASS =
    "$GLGSV,1,1,04,65,45,123,40,66,30,234,35,67,20,345,30,68,10,056,25*6E";
const std::string GSV_GALILEO =
    "$GAGSV,2,1,05,02,42,279,22,04,22,112,36,06,12,135,33,25,30,211,35,7*7C";
const std::string GSV_BEIDOU =
    "$GBGSV,2,1,05,02,13,116,42,19,52,119,45,29,18,114,43,35,75,126,45,1*76";

const std::string RMC_GPS =
    "$GPRMC,125740.00,A,4852.60000,S,12323.60000,W,0.091,,100426,,,D,V*06";
const std::string RMC_MULTI =
    "$GNRMC,125740.00,A,4852.60000,S,12323.60000,W,0.091,,100426,,,D,V*18";

const std::string BAD_NOISE = "noise from another protocol";
const std::string BAD_CRCBAD =
    "$GNRMC,125740.00,A,4852.60000,S,12323.60000,W,0.091,,100426,,,D,V*FF";
const std::string BAD_CRCMISSING =
    "$GNGGA,125740.00,4852.60000,S,12323.60000,W,2,12,1.28,237.1,M,25.1,M,,";
const std::string BAD_TRUNCATED = "$GN";

class mock_uart_iface : public uhd::uart_iface
{
public:
    mock_uart_iface(
        std::string detection_sentence, std::vector<std::string> buffered_sentences)
        : _detection_sentence(std::move(detection_sentence))
        , _buffered_sentences(std::move(buffered_sentences))
    {
    }

    void write_uart(const std::string& buf) override
    {
        _writes.push_back(buf);
    }

    std::string read_uart(double timeout) override
    {
        if (timeout <= 0.0) {
            if (_writes.empty()) {
                return {};
            }

            if (_buffered_index < _buffered_sentences.size()) {
                return _buffered_sentences.at(_buffered_index++);
            }

            return {};
        }

        if (!_returned_detection_sentence) {
            _returned_detection_sentence = true;
            return _detection_sentence;
        }

        return {};
    }

private:
    std::string _detection_sentence;
    std::vector<std::string> _buffered_sentences;
    std::vector<std::string> _writes;
    size_t _buffered_index            = 0;
    bool _returned_detection_sentence = false;
};

struct captured_log_state
{
    std::mutex mutex;
    std::vector<uhd::log::logging_info> entries;
};

captured_log_state& get_captured_log_state()
{
    static captured_log_state state;
    return state;
}

void ensure_warning_logger()
{
    static bool logger_installed = false;

    if (!logger_installed) {
        uhd::log::add_logger(
            "gps_ctrl_test_logger", [](const uhd::log::logging_info& info) {
                auto& state = get_captured_log_state();
                std::lock_guard<std::mutex> lock(state.mutex);
                state.entries.push_back(info);
            });
        uhd::log::set_logger_level("gps_ctrl_test_logger", uhd::log::warning);
        logger_installed = true;
    }
}

void clear_captured_logs()
{
    auto& state = get_captured_log_state();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.entries.clear();
}

std::vector<std::string> get_gps_warning_messages()
{
    auto& state = get_captured_log_state();
    std::lock_guard<std::mutex> lock(state.mutex);
    std::vector<std::string> messages;
    for (const auto& entry : state.entries) {
        if (entry.component == "GPS" && entry.verbosity == uhd::log::warning) {
            messages.push_back(entry.message);
        }
    }
    return messages;
}

} // namespace


BOOST_AUTO_TEST_CASE(test_nmea_legacy)
{
    auto uart = std::make_shared<mock_uart_iface>(RMC_GPS,
        std::vector<std::string>{
            GGA_GPS,
            RMC_GPS,
        });

    auto gps = uhd::gps_ctrl::make(uart);

    BOOST_CHECK(gps->gps_detected());
    BOOST_CHECK_EQUAL(gps->get_sensor("gps_gprmc").value, RMC_GPS);
    BOOST_CHECK_EQUAL(gps->get_sensor("gps_gpgga").value, GGA_GPS);
    BOOST_CHECK(gps->get_sensor("gps_locked").to_bool());
}

BOOST_AUTO_TEST_CASE(test_nmea_malformed_messages)
{
    auto uart = std::make_shared<mock_uart_iface>(RMC_MULTI,
        std::vector<std::string>{
            BAD_NOISE,
            BAD_CRCBAD,
            BAD_CRCMISSING,
            BAD_TRUNCATED,
            GGA_MULTI,
            RMC_MULTI,
        });

    auto gps = uhd::gps_ctrl::make(uart);

    BOOST_CHECK(gps->gps_detected());

    BOOST_CHECK_EQUAL(gps->get_sensor("gps_gprmc").value, RMC_MULTI);
    BOOST_CHECK_EQUAL(gps->get_sensor("gps_gpgga").value, GGA_MULTI);
}

BOOST_AUTO_TEST_CASE(test_nmea_multi_messages)
{
    ensure_warning_logger();
    clear_captured_logs();

    auto uart = std::make_shared<mock_uart_iface>(RMC_MULTI,
        std::vector<std::string>{
            GSV_GPS,
            GSV_GLONASS,
            GSV_GALILEO,
            GSV_BEIDOU,
            GGA_MULTI,
            RMC_MULTI,
        });

    auto gps = uhd::gps_ctrl::make(uart);

    BOOST_CHECK(gps->gps_detected());

    BOOST_CHECK_EQUAL(gps->get_sensor("gps_gprmc").value, RMC_MULTI);
    BOOST_CHECK_EQUAL(gps->get_sensor("gps_gpgga").value, GGA_MULTI);
    BOOST_CHECK(gps->get_sensor("gps_locked").to_bool());
    BOOST_CHECK(get_gps_warning_messages().empty());
}
