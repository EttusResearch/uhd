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

#include <uhd/utils/props.hpp>

using namespace uhd;

named_prop_t::named_prop_t(
    const wax::obj &key,
    const std::string &name
):
    key(key),
    name(name)
{
    /* NOP */
}

named_prop_t named_prop_t::extract(
    const wax::obj &key,
    const std::string &name
){
    if (key.type() == typeid(named_prop_t)){
        return key.as<named_prop_t>();
    }
    return named_prop_t(key, name);
}
