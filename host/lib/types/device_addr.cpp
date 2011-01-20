//
// Copyright 2011 Ettus Research LLC
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

#include <uhd/types/device_addr.hpp>
#include <boost/algorithm/string.hpp> //for trim
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <stdexcept>
#include <sstream>

using namespace uhd;

static const std::string arg_delim = ",";
static const std::string pair_delim = "=";

static std::string trim(const std::string &in){
    return boost::algorithm::trim_copy(in);
}

#define tokenizer(inp, sep) \
    boost::tokenizer<boost::char_separator<char> > \
    (inp, boost::char_separator<char>(sep.c_str()))

device_addr_t::device_addr_t(const std::string &args){
    BOOST_FOREACH(const std::string &pair, tokenizer(args, arg_delim)){
        if (trim(pair) == "") continue;
        std::string key;
        BOOST_FOREACH(const std::string &tok, tokenizer(pair, pair_delim)){
            if (key.empty()) key = tok;
            else{
                this->set(trim(key), trim(tok));
                goto continue_next_arg;
            }
        }
        throw std::runtime_error("invalid args string: "+args);
        continue_next_arg: continue;
    }
}

std::string device_addr_t::to_pp_string(void) const{
    if (this->size() == 0) return "Empty Device Address";

    std::stringstream ss;
    ss << "Device Address:" << std::endl;
    BOOST_FOREACH(std::string key, this->keys()){
        ss << boost::format("    %s: %s") % key % this->get(key) << std::endl;
    }
    return ss.str();
}

std::string device_addr_t::to_string(void) const{
    std::string args_str;
    size_t count = 0;
    BOOST_FOREACH(const std::string &key, this->keys()){
        args_str += ((count++)? arg_delim : "") + key + pair_delim + this->get(key);
    }
    return args_str;
}
