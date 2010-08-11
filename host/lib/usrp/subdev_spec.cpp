//
// Copyright 2010 Ettus Research LLC
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

#include <uhd/usrp/subdev_spec.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <stdexcept>
#include <sstream>

using namespace uhd;
using namespace uhd::usrp;

subdev_spec_pair_t::subdev_spec_pair_t(
    const std::string &db_name, const std::string &sd_name
):
    db_name(db_name),
    sd_name(sd_name)
{
    /* NOP */
}

subdev_spec_t::subdev_spec_t(const std::string &markup){
    std::vector<std::string> pairs;
    boost::split(pairs, markup, boost::is_any_of("\t "));
    BOOST_FOREACH(const std::string &pair, pairs){
        if (pair == "") continue;
        std::vector<std::string> db_sd;
        boost::split(db_sd, pair, boost::is_any_of(":"));
        switch(db_sd.size()){
        case 1: this->push_back(subdev_spec_pair_t("", db_sd.front())); break;
        case 2: this->push_back(subdev_spec_pair_t(db_sd.front(), db_sd.back())); break;
        default: throw std::runtime_error("invalid subdev-spec markup string: "+markup);
        }
    }
}

std::string subdev_spec_t::to_pp_string(void) const{
    if (this->size() == 0) return "Empty Subdevice Specification";

    std::stringstream ss;
    size_t count = 0;
    ss << "Subdevice Specification:" << std::endl;
    BOOST_FOREACH(const subdev_spec_pair_t &pair, *this){
        std::string db_name = pair.db_name;
        if (db_name == "") db_name = "0";
        std::string sd_name = pair.sd_name;
        if (sd_name == "") sd_name = "0";
        ss << boost::format(
            "    Channel %d: Daughterboard %s, Subdevice %s"
        ) % (count++) % db_name % sd_name << std::endl;
    }
    return ss.str();
}

std::string subdev_spec_t::to_string(void) const{
    std::string markup;
    size_t count = 0;
    BOOST_FOREACH(const subdev_spec_pair_t &pair, *this){
        markup += ((count++)? " " : "") + pair.db_name + ":" + pair.sd_name;
    }
    return markup;
}
