//
// Copyright 2017 Ettus Research (National Instruments Corp.)
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

// Note: Including this file requires C++11 features enabled.

#ifndef INCLUDED_UHD_UTILS_LOG_ADD_HPP
#define INCLUDED_UHD_UTILS_LOG_ADD_HPP

#include <uhd/config.hpp>
#include <uhd/utils/log.hpp>
#include <functional>

namespace uhd {
    namespace log {

        /*! Logging function type
         *
         * Every logging_backend has to define a function with this signature.
         * Can be added to the logging core.
         */
        typedef std::function<void(const uhd::log::logging_info&)> log_fn_t ;

        /*! Add logging backend to the log system
         *
         * \param key Identifies the logging backend in the logging core
         * \param logger_fn function which actually logs messages to this backend
         */
        UHD_API void add_logger(const std::string &key, log_fn_t logger_fn);
    }
} /* namespace uhd::log */

#endif /* INCLUDED_UHD_UTILS_LOG_ADD_HPP */
