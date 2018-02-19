//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
