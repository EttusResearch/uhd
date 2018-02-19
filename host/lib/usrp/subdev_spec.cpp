//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/exception.hpp>
#include <boost/algorithm/string.hpp> //for split
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <vector>

using namespace uhd;
using namespace uhd::usrp;

#define pair_tokenizer(inp) \
    boost::tokenizer<boost::char_separator<char> > \
    (inp, boost::char_separator<char>(" "))

subdev_spec_pair_t::subdev_spec_pair_t(
    const std::string &db_name, const std::string &sd_name
):
    db_name(db_name),
    sd_name(sd_name)
{
    /* NOP */
}

bool usrp::operator==(const subdev_spec_pair_t &lhs, const subdev_spec_pair_t &rhs){
    return (lhs.db_name == rhs.db_name) and (lhs.sd_name == rhs.sd_name);
}

bool subdev_spec_pair_t::operator==(const subdev_spec_pair_t &other){
    return (other.db_name == db_name) and (other.sd_name == sd_name);
}

bool subdev_spec_pair_t::operator!=(const subdev_spec_pair_t &other){
    return (other.db_name != db_name) or (other.sd_name != sd_name);
}

subdev_spec_t::subdev_spec_t(const std::string &markup){
    for(const std::string &pair:  pair_tokenizer(markup)){
        if (pair.empty()) continue;
        std::vector<std::string> db_sd; boost::split(db_sd, pair, boost::is_any_of(":"));
        switch(db_sd.size()){
        case 1: this->push_back(subdev_spec_pair_t("", db_sd.front())); break;
        case 2: this->push_back(subdev_spec_pair_t(db_sd.front(), db_sd.back())); break;
        default: throw uhd::value_error("invalid subdev-spec markup string: "+markup);
        }
    }
}

std::string subdev_spec_t::to_pp_string(void) const{
    if (this->size() == 0) return "Empty Subdevice Specification";

    std::stringstream ss;
    size_t count = 0;
    ss << "Subdevice Specification:" << std::endl;
    for(const subdev_spec_pair_t &pair:  *this){
        ss << boost::format(
            "    Channel %d: Daughterboard %s, Subdevice %s"
        ) % (count++) % pair.db_name % pair.sd_name << std::endl;
    }
    return ss.str();
}

std::string subdev_spec_t::to_string(void) const{
    std::string markup;
    size_t count = 0;
    for(const subdev_spec_pair_t &pair:  *this){
        markup += ((count++)? " " : "") + pair.db_name + ":" + pair.sd_name;
    }
    return markup;
}
