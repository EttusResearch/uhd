//
// Copyright 2010-2011 Ettus Research LLC
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

#include <uhd/utils/warning.hpp>
#include <uhd/exception.hpp>
#include <boost/tokenizer.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/types/dict.hpp>
#include <boost/foreach.hpp>
#include <sstream>
#include <iostream>
#include <vector>

using namespace uhd;

#define tokenizer(inp, sep) \
    boost::tokenizer<boost::char_separator<char> > \
    (inp, boost::char_separator<char>(sep))

/***********************************************************************
 * Registry implementation
 **********************************************************************/
//create the registry for the handlers
typedef uhd::dict<std::string, warning::handler_t> registry_t;
UHD_SINGLETON_FCN(registry_t, get_registry)

//the default warning handler
static void stderr_warning(const std::string &msg){
    std::cerr << msg;
}

//register a default handler
UHD_STATIC_BLOCK(warning_register_default){
    warning::register_handler("default", &stderr_warning);
}

/***********************************************************************
 * Post + format
 **********************************************************************/
void warning::post(const std::string &msg){
    std::stringstream ss;

    //format the warning message
    ss << std::endl << "Warning:" << std::endl;
    BOOST_FOREACH(const std::string &line, tokenizer(msg, "\n")){
        ss << "    " << line << std::endl;
    }

    //post the formatted message
    BOOST_FOREACH(const std::string &name, get_registry().keys()){
        get_registry()[name](ss.str());
    }
}

/***********************************************************************
 * Registry accessor functions
 **********************************************************************/
void warning::register_handler(
    const std::string &name, const handler_t &handler
){
    get_registry()[name] = handler;
}

warning::handler_t warning::unregister_handler(const std::string &name){
    if (not get_registry().has_key(name)) throw uhd::key_error(
        "The warning registry does not have a handler registered to " + name
    );
    return get_registry().pop(name);
}

const std::vector<std::string> warning::registry_names(void){
    return get_registry().keys();
}
