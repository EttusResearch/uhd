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

#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <sstream>
#include <iostream>

/***********************************************************************
 * Helper functions
 **********************************************************************/
#define tokenizer(inp, sep) \
    boost::tokenizer<boost::char_separator<char> > \
    (inp, boost::char_separator<char>(sep))

static void msg_to_cout(const std::string &msg){
    std::stringstream ss;

    static bool just_had_a_newline = true;
    BOOST_FOREACH(char ch, msg){
        if (just_had_a_newline){
            just_had_a_newline = false;
            ss << "-- ";
        }
        if (ch == '\n'){
            just_had_a_newline = true;
        }
        ss << ch;
    }

    std::cout << ss.str() << std::flush;
}

static void msg_to_cerr(const std::string &title, const std::string &msg){
    std::stringstream ss;

    ss << std::endl << title << ":" << std::endl;
    BOOST_FOREACH(const std::string &line, tokenizer(msg, "\n")){
        ss << "    " << line << std::endl;
    }

    std::cerr << ss.str() << std::flush;
}

/***********************************************************************
 * Global resources for the messenger
 **********************************************************************/
struct msg_resource_type{
    boost::mutex mutex;
    uhd::msg::handler_t handler;
};

UHD_SINGLETON_FCN(msg_resource_type, msg_rs);

/***********************************************************************
 * Setup the message handlers
 **********************************************************************/
void uhd::msg::register_handler(const handler_t &handler){
    boost::mutex::scoped_lock lock(msg_rs().mutex);
    msg_rs().handler = handler;
}

static void default_msg_handler(uhd::msg::type_t type, const std::string &msg){
    switch(type){
    case uhd::msg::fastpath:
        std::cerr << msg << std::flush;
        break;

    case uhd::msg::status:
        msg_to_cout(msg);
        UHD_LOG << "Status message" << std::endl << msg;
        break;

    case uhd::msg::warning:
        msg_to_cerr("UHD Warning", msg);
        UHD_LOG << "Warning message" << std::endl << msg;
        break;

    case uhd::msg::error:
        msg_to_cerr("UHD Error", msg);
        UHD_LOG << "Error message" << std::endl << msg;
        break;
    }
}

UHD_STATIC_BLOCK(msg_register_default_handler){
    uhd::msg::register_handler(&default_msg_handler);
}

/***********************************************************************
 * The message object implementation
 **********************************************************************/
struct uhd::msg::_msg::impl{
    std::ostringstream ss;
    type_t type;
};

uhd::msg::_msg::_msg(const type_t type){
    _impl = UHD_PIMPL_MAKE(impl, ());
    _impl->type = type;
}

uhd::msg::_msg::~_msg(void){
    boost::mutex::scoped_lock lock(msg_rs().mutex);
    msg_rs().handler(_impl->type, _impl->ss.str());
}

std::ostream & uhd::msg::_msg::operator()(void){
    return _impl->ss;
}
