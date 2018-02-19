//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/filters.hpp>

using namespace uhd;

std::ostream& uhd::operator<<(std::ostream& os, filter_info_base& f)
{
    return os << f.to_pp_string();
}

std::string filter_info_base::to_pp_string()
{
    std::ostringstream os;
    os << "[filter_info_base]" << std::endl;
    switch(_type){
        case ANALOG_LOW_PASS:
            os << "type: " << "Analog Low-pass" << std::endl;
            break;
        case ANALOG_BAND_PASS:
            os << "type: " << "Analog Band-pass" << std::endl;
            break;
        case DIGITAL_I16:
            os << "type: " << "Digital (i16)" << std::endl;
            break;
        case DIGITAL_FIR_I16:
            os << "type: " << "Digital FIR (i16)" << std::endl;
            break;
        default:
            os << "type: " << "Unknown type!" << std::endl;
            break;
        }

    os << "bypass enable: " << _bypass << std::endl
        <<"position index: " << _position_index << std::endl;

    std::string str =  os.str();
    return str;
}

std::string analog_filter_base::to_pp_string()
{
    std::ostringstream os;
    os << filter_info_base::to_pp_string() <<
        "\t[analog_filter_base]" << std::endl <<
        "\tdesc: " << _analog_type << std::endl;
    return std::string(os.str());

}

std::string analog_filter_lp::to_pp_string()
{
    std::ostringstream os;
    os << analog_filter_base::to_pp_string() <<
        "\t\t[analog_filter_lp]" << std::endl <<
        "\t\tcutoff: " << _cutoff << std::endl <<
        "\t\trolloff: " << _rolloff << std::endl;
    return std::string(os.str());
}
