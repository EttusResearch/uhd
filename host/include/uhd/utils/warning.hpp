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

#ifndef INCLUDED_UHD_UTILS_WARNING_HPP
#define INCLUDED_UHD_UTILS_WARNING_HPP

#include <uhd/config.hpp>
#include <boost/function.hpp>
#include <vector>
#include <string>

namespace uhd{ namespace warning{

    //! Callback function type for a message handler
    typedef boost::function<void(std::string)> handler_t;

    /*!
     * Post a warning message to all registered handlers.
     * \param msg the multiline warning message
     */
    UHD_API void post(const std::string &msg);

    /*!
     * Register a new handler with this name.
     * If the name was already registered for this name,
     * the old registered handler will be replaced.
     * \param name a unique name for this handler
     * \param handler the callback handler function
     */
    UHD_API void register_handler(const std::string &name, const handler_t &handler);

    /*!
     * Unregister a handler for this name.
     * \param name a unique name for a registered handler
     * \return the handler that was registered
     * \throw error when the name was not found in the registry
     */
    UHD_API handler_t unregister_handler(const std::string &name);

    /*!
     * Get a list of registered handler names.
     * \return a vector of unique string names
     */
    UHD_API const std::vector<std::string> registry_names(void);

}} //namespace uhd::warning

#endif /* INCLUDED_UHD_UTILS_WARNING_HPP */
