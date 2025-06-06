//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/time_spec.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <string>

using namespace uhd;

std::string rx_metadata_t::to_pp_string(bool compact) const
{
    std::stringstream ss;

    if (compact) {
        if (has_time_spec) {
            ss << "Time: " << time_spec.get_real_secs() << " s\n";
        }
        if (more_fragments) {
            ss << "Fragmentation offset: " << fragment_offset << "\n";
        }
        if (start_of_burst) {
            ss << "Start of burst.\n";
        }
        if (end_of_burst) {
            ss << "End of burst.\n";
        }
        if (error_code != ERROR_CODE_NONE) {
            ss << strerror() << "\n";
        }
    } else {
        ss << "Has timespec: " << (has_time_spec ? "Yes" : "No")
           << "\tTime of first sample: " << time_spec.get_real_secs()
           << "\nFragmented: " << (more_fragments ? "Yes" : "No")
           << "  Fragmentation offset: " << fragment_offset
           << "\nStart of burst: " << (start_of_burst ? "Yes" : "No")
           << "\tEnd of burst: " << (end_of_burst ? "Yes" : "No")
           << "\nError Code: " << strerror()
           << "\tOut of sequence: " << (out_of_sequence ? "Yes" : "No");
    }

    return ss.str();
}

std::string rx_metadata_t::strerror() const
{
    std::string errstr = "";
    switch (this->error_code) {
        case ERROR_CODE_NONE:
            errstr = "ERROR_CODE_NONE";
            break;
        case ERROR_CODE_TIMEOUT:
            errstr = "ERROR_CODE_TIMEOUT";
            break;
        case ERROR_CODE_LATE_COMMAND:
            errstr = "ERROR_CODE_LATE_COMMAND";
            break;
        case ERROR_CODE_BROKEN_CHAIN:
            errstr = "ERROR_CODE_BROKEN_CHAIN (Expected another stream command)";
            break;
        case ERROR_CODE_OVERFLOW:
            errstr = "ERROR_CODE_OVERFLOW ";
            errstr += (this->out_of_sequence ? "(Out of sequence error)" : "(Overflow)");
            break;
        case ERROR_CODE_ALIGNMENT:
            errstr = "ERROR_CODE_ALIGNMENT (Multi-channel alignment failed)";
            break;
        case ERROR_CODE_BAD_PACKET:
            errstr = "ERROR_CODE_BAD_PACKET";
            break;
        default:
            errstr =
                std::string(str(boost::format("Unknown error code: 0x%x") % error_code));
    }

    return errstr;
}

std::string async_metadata_t::to_pp_string(bool compact) const
{
    std::stringstream ss;

    if (compact) {
        if (has_time_spec) {
            ss << "Time: " << time_spec.get_real_secs() << " s\n";
        }
        ss << "Channel: " << channel << "\n";
        ss << strevent() << "\n";
    } else {
        ss << "Has timespec: " << (has_time_spec ? "Yes" : "No")
           << "\tTime of first sample: " << time_spec.get_real_secs()
           << "\nChannel: " << channel << "\nEvent Code: " << strevent();
    }

    return ss.str();
}

std::string async_metadata_t::strevent() const
{
    std::string eventstr = "";
    switch (event_code) {
        case EVENT_CODE_BURST_ACK:
            eventstr = "EVENT_CODE_BURST_ACK";
            break;
        case EVENT_CODE_UNDERFLOW:
            eventstr = "EVENT_CODE_UNDERFLOW";
            break;
        case EVENT_CODE_SEQ_ERROR:
            eventstr = "EVENT_CODE_SEQ_ERROR";
            break;
        case EVENT_CODE_TIME_ERROR:
            eventstr = "EVENT_CODE_TIME_ERROR";
            break;
        case EVENT_CODE_UNDERFLOW_IN_PACKET:
            eventstr = "EVENT_CODE_UNDERFLOW_IN_PACKET ";
            break;
        case EVENT_CODE_SEQ_ERROR_IN_BURST:
            eventstr = "EVENT_CODE_SEQ_ERROR_IN_BURST";
            break;
        case EVENT_CODE_USER_PAYLOAD:
            eventstr = std::string(str(
                boost::format("EVENT_CODE_USER_PAYLOAD 0x%08x 0x%08x 0x%08x 0x%08x")
                % user_payload[0] % user_payload[1] % user_payload[2] % user_payload[3]));
            break;
        default:
            eventstr =
                std::string(str(boost::format("Unknown event code: 0x%x") % event_code));
    }

    return eventstr;
}
