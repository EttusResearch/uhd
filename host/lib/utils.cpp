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

#include <uhd/utils/assert.hpp>
#include <uhd/utils/props.hpp>
#include <stdexcept>

using namespace uhd;

/***********************************************************************
 * Assert
 **********************************************************************/
assert_error::assert_error(const std::string &what) : std::runtime_error(what){
    /* NOP */
}

/***********************************************************************
 * Props
 **********************************************************************/
named_prop_t::named_prop_t(
    const wax::obj &key_,
    const std::string &name_
){
    key = key_;
    name = name_;
}

typedef boost::tuple<wax::obj, std::string> named_prop_tuple;

named_prop_tuple uhd::extract_named_prop(
    const wax::obj &key,
    const std::string &name
){
    if (key.type() == typeid(named_prop_t)){
        named_prop_t np = key.as<named_prop_t>();
        return named_prop_tuple(np.key, np.name);
    }
    return named_prop_tuple(key, name);
}
