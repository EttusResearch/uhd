//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Note: Including this file requires C++11 features enabled.

#pragma once

#include <uhd/config.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/log_add_impl.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <functional>

namespace uhd { namespace log {

/*! Legacy logging info structure
 */
struct logging_info
{
    logging_info() : verbosity(uhd::log::off), line(0) {}
    logging_info(const boost::posix_time::ptime& time_,
        const uhd::log::severity_level& verbosity_,
        const std::string& file_,
        const unsigned int& line_,
        const std::string& component_,
        const std::thread::id& thread_id_)
        : time(time_)
        , verbosity(verbosity_)
        , file(file_)
        , line(line_)
        , component(component_)
        , thread_id(thread_id_)
    { /* nop */
    }

    boost::posix_time::ptime time;
    uhd::log::severity_level verbosity;
    std::string file;
    unsigned int line;
    std::string component;
    std::thread::id thread_id;
    std::string message;
};

/*! Logging function type
 *
 * Every logging_backend has to define a function with this signature.
 * Can be added to the logging core.
 */
using log_fn_legacy_t = std::function<void(const uhd::log::logging_info&)>;

/*! Add logging backend to the log system
 *
 * \param key Identifies the logging backend in the logging core
 * \param logger_fn function which actually logs messages to this backend
 */
[[deprecated("This function is deprecated. Please use add_logger with log_fn_t "
             "instead.")]] inline void
add_logger(const std::string& key, log_fn_legacy_t logger_fn)
{
    uhd::log::add_logger(key, [logger_fn](const uhd::log::detail::logging_info& info) {
        // Convert std::chrono time_point to boost::posix_time::ptime
        auto time_t_val = std::chrono::system_clock::to_time_t(info.time);
        auto us         = std::chrono::duration_cast<std::chrono::microseconds>(
                      info.time.time_since_epoch())
                  % 1000000;
        boost::posix_time::ptime ptime =
            boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1))
            + boost::posix_time::seconds(time_t_val)
            + boost::posix_time::microseconds(us.count());
        uhd::log::logging_info legacy_info(
            ptime, info.verbosity, info.file, info.line, info.component, info.thread_id);
        legacy_info.message = info.message;
        logger_fn(legacy_info);
    });
}


}} /* namespace uhd::log */
