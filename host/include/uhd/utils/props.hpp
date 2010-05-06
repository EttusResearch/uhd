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

#ifndef INCLUDED_UHD_UTILS_PROPS_HPP
#define INCLUDED_UHD_UTILS_PROPS_HPP

#include <uhd/config.hpp>
#include <uhd/wax.hpp>
#include <uhd/utils/exception.hpp>
#include <boost/tuple/tuple.hpp>
#include <stdexcept>
#include <vector>
#include <string>

namespace uhd{

    //! The type for a vector of property names
    typedef std::vector<std::string> prop_names_t;

    /*!
     * A named prop struct holds a key and a name.
     * Allows properties to be sub-sectioned by name.
     */
    struct UHD_API named_prop_t{
        wax::obj key;
        std::string name;

        /*!
         * Create a new named prop from key and name.
         * \param key the property key
         * \param name the string name
         */
        named_prop_t(const wax::obj &key, const std::string &name);
    };

    /*!
     * Utility function to separate a named property into its components.
     * \param key a reference to the prop object
     * \param name a reference to the name object
     * \return a tuple that can be used with boost::tie
     */
    UHD_API boost::tuple<wax::obj, std::string> extract_named_prop(
        const wax::obj &key,
        const std::string &name = ""
    );

    /*!
     * Throw when getting a not-implemented or write-only property.
     * Throw-site information will be included with this error.
     */
    #define UHD_THROW_PROP_GET_ERROR() \
        throw std::runtime_error(UHD_THROW_SITE_INFO("cannot get this property"))

    /*!
     * Throw when setting a not-implemented or read-only property.
     * Throw-site information will be included with this error.
     */
    #define UHD_THROW_PROP_SET_ERROR() \
        throw std::runtime_error(UHD_THROW_SITE_INFO("cannot set this property"))

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_PROPS_HPP */
