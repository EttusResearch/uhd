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

#ifndef INCLUDED_UHD_USRP_PROPS_COMMON_HPP
#define INCLUDED_UHD_USRP_PROPS_COMMON_HPP

#include <uhd/config.hpp>
#include <uhd/wax.hpp>
#include <boost/tuple/tuple.hpp>
#include <vector>
#include <string>

namespace uhd{

    //typedef for handling named properties
    typedef std::vector<std::string> prop_names_t;
    typedef boost::tuple<wax::obj, std::string> named_prop_t;

    /*!
     * Utility function to separate a named property into its components.
     * \param key a reference to the prop object
     * \param name a reference to the name object
     */
    inline UHD_API named_prop_t //must be exported as part of the api to work (TODO move guts to cpp file)
    extract_named_prop(const wax::obj &key, const std::string &name = ""){
        if (key.type() == typeid(named_prop_t)){
            return key.as<named_prop_t>();
        }
        return named_prop_t(key, name);
    }

} //namespace uhd

#endif /* INCLUDED_UHD_USRP_PROPS_COMMON_HPP */
