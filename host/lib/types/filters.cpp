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
