//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/optional.hpp>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <thread>

/*! \file log.hpp
 *
 * \section loghpp_logging The UHD logging facility
 *
 * The logger enables UHD library code to easily log events into a file and
 * display messages above a certain level in the terminal.
 * Log entries are time-stamped and stored with file, line, and function.
 * Each call to the UHD_LOG macros is thread-safe. Each thread will aquire the
 * lock for the logger.
 *
 * Note: More information on the logging subsystem can be found on
 * \ref page_logging.
 *
 * To disable console logging completely at compile time specify
 * `-DUHD_LOG_CONSOLE_DISABLE` during configuration with CMake.
 *
 * By default no file logging will occur. Set a log file path:
 *  - at compile time by specifying `-DUHD_LOG_FILE=$file_path`
 *  - and/or override at runtime by setting the environment variable
 *    `UHD_LOG_FILE`
 *
 * \subsection loghpp_levels Log levels
 *
 * See also \ref logging_levels.
 *
 * All log messages with verbosity greater than or equal to the log level
 * (in other words, as often or less often than the current log level)
 * are recorded to std::clog and/or the log file.
 * Log levels can be specified using string or numeric values of
 * uhd::log::severity_level.
 *
 * The minimum log level is defined by `-DUHD_LOG_MIN_LEVEL` at compile time,
 * and this value can be increased at runtime by specifying the `UHD_LOG_LEVEL`
 * environment variable. This minimum logging level applies to any form of
 * runtime logging. Thus for example if this minimum is set to 3 (`info`), then
 * during runtime no logging at levels below 3 can be provided.
 *
 * The following set the minimum logging level to 3 (`info`):
 *   - Example pre-processor define: `-DUHD_LOG_MIN_LEVEL=3`
 *   - Example pre-processor define: `-DUHD_LOG_MIN_LEVEL=info`
 *   - Example environment variable: `export UHD_LOG_LEVEL=3`
 *   - Example environment variable: `export UHD_LOG_LEVEL=info`
 *
 * The actual log level for console and file logging can be configured by
 * setting `UHD_LOG_CONSOLE_LEVEL` or `UHD_LOG_FILE_LEVEL`, respectively. The
 * default values for these variables can be defined using the cmake flags
 * `-DUHD_LOG_CONSOLE_LEVEL` and `-DUHD_LOG_FILE_LEVEL`, respectively.
 *
 * These variables can be the name of a verbosity enum or integer value:
 *   - Example pre-processor define: `-DUHD_LOG_CONSOLE_LEVEL=3`
 *   - Example pre-processor define: `-DUHD_LOG_CONSOLE_LEVEL=info`
 *   - Example environment variable: `export UHD_LOG_CONSOLE_LEVEL=3`
 *   - Example environment variable: `export UHD_LOG_CONSOLE_LEVEL=info`
 *
 * The `UHD_LOG_FILE_LEVEL` variable can be used in the same way.
 *
 * \subsection loghpp_formatting Log formatting
 *
 * The log format for messages going into a log file is CSV.
 * All log messages going into a logfile will contain following fields:
 * - timestamp
 * - thread-id
 * - source-file + line information
 * - severity level
 * - component/channel information which logged the information
 * - the actual log message
 *
 * The log format of log messages displayed on the terminal is plain text with
 * space separated tags prepended.
 * For example:
 *    - `[INFO] [X300] This is a informational log message`
 *
 * The log format for log output on the console by using these preprocessor
 * defines in CMake:
 * - `-DUHD_LOG_CONSOLE_TIME` adds a timestamp [2017-01-01 00:00:00.000000]
 * - `-DUHD_LOG_CONSOLE_THREAD` adds a thread-id `[0x001234]`
 * - `-DUHD_LOG_CONSOLE_SRC` adds a sourcefile and line tag `[src_file:line]`
 */

/*
 * Advanced logging macros
 * UHD_LOG_MIN_LEVEL definitions
 * trace: 0
 * debug: 1
 * info: 2
 * warning: 3
 * error: 4
 * fatal: 5
 */

namespace uhd { namespace log {
/*! Logging severity levels
 *
 * Either numeric value or string can be used to define loglevel in
 * CMake and environment variables
 */
enum severity_level {
    trace   = 0, /**< displays every available log message */
    debug   = 1, /**< displays most log messages necessary for debugging internals */
    info    = 2, /**< informational messages about setup and what is going on*/
    warning = 3, /**< something is not right but operation can continue */
    error   = 4, /**< something has gone wrong */
    fatal   = 5, /**< something has gone horribly wrong */
    off     = 6, /**< logging is turned off */
};

/*! Parses a `severity_level` from a string. If a value could not be parsed,
 * returns none.
 */
boost::optional<uhd::log::severity_level> UHD_API parse_log_level_from_string(
    const std::string& log_level_str);

/*! Logging info structure
 *
 * Information needed to create a log entry is fully contained in the
 * logging_info structure.
 */
struct UHD_API logging_info
{
    logging_info() : verbosity(uhd::log::off) {}
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

/*! Set the global log level
 *
 * The global log level gets applied before the specific log level.
 * So, if the global log level is 'info', no logger can can print
 * messages at level 'debug' or below.
 */
UHD_API void set_log_level(uhd::log::severity_level level);

/*! Set the log level for the console logger (if defined).
 *
 * Short-hand for `set_logger_level("console", level);`
 */
UHD_API void set_console_level(uhd::log::severity_level level);

/*! Set the log level for the file logger (if defined)
 *
 * Short-hand for `set_logger_level("file", level);`
 */
UHD_API void set_file_level(uhd::log::severity_level level);

/*! Set the log level for any specific logger.
 *
 * \param logger Name of the logger
 * \param level New log level for this logger.
 *
 * \throws uhd::key_error if \p logger was not defined
 */
UHD_API void set_logger_level(const std::string& logger, uhd::log::severity_level level);
}} // namespace uhd::log

//! \cond
//! Internal logging macro to be used in other macros
#define _UHD_LOG_INTERNAL(component, level) \
    uhd::_log::log(level, __FILE__, __LINE__, component, std::this_thread::get_id())
//! \endcond

// macro-style logging (compile-time determined)
#if UHD_LOG_MIN_LEVEL < 1
#    define UHD_LOG_TRACE(component, message) \
        _UHD_LOG_INTERNAL(component, uhd::log::trace) << message;
#else
#    define UHD_LOG_TRACE(component, message)
#endif

#if UHD_LOG_MIN_LEVEL < 2
#    define UHD_LOG_DEBUG(component, message) \
        _UHD_LOG_INTERNAL(component, uhd::log::debug) << message;
#else
#    define UHD_LOG_DEBUG(component, message)
#endif

#if UHD_LOG_MIN_LEVEL < 3
#    define UHD_LOG_INFO(component, message) \
        _UHD_LOG_INTERNAL(component, uhd::log::info) << message;
#else
#    define UHD_LOG_INFO(component, message)
#endif

#if UHD_LOG_MIN_LEVEL < 4
#    define UHD_LOG_WARNING(component, message) \
        _UHD_LOG_INTERNAL(component, uhd::log::warning) << message;
#else
#    define UHD_LOG_WARNING(component, message)
#endif

#if UHD_LOG_MIN_LEVEL < 5
#    define UHD_LOG_ERROR(component, message) \
        _UHD_LOG_INTERNAL(component, uhd::log::error) << message;
#else
#    define UHD_LOG_ERROR(component, message)
#endif

#define UHD_LOG_THROW(exception_type, component, message) \
    {                                                     \
        std::ostringstream __ss;                          \
        __ss << message;                                  \
        UHD_LOG_ERROR(component, __ss.str());             \
        throw exception_type(__ss.str());                 \
    }

#if UHD_LOG_MIN_LEVEL < 6
#    define UHD_LOG_FATAL(component, message) \
        _UHD_LOG_INTERNAL(component, uhd::log::fatal) << message;
#else
#    define UHD_LOG_FATAL(component, message)
#endif

#define RFNOC_LOG_TRACE(message) UHD_LOG_TRACE(this->get_unique_id(), message)
#define RFNOC_LOG_DEBUG(message) UHD_LOG_DEBUG(this->get_unique_id(), message)
#define RFNOC_LOG_INFO(message) UHD_LOG_INFO(this->get_unique_id(), message)
#define RFNOC_LOG_WARNING(message) UHD_LOG_WARNING(this->get_unique_id(), message)
#define RFNOC_LOG_ERROR(message) UHD_LOG_ERROR(this->get_unique_id(), message)
#define RFNOC_LOG_FATAL(message) UHD_LOG_FATAL(this->get_unique_id(), message)

#ifndef UHD_LOG_FASTPATH_DISABLE
//! Extra-fast logging macro for when speed matters.
// No metadata is tracked. Only the message is displayed. This does not go
// through the regular backends. Mostly used for printing the UOSDL characters
// during streaming.
#    define UHD_LOG_FASTPATH(message) uhd::_log::log_fastpath(message);
#else
#    define UHD_LOG_FASTPATH(message)
#endif

// iostream-style logging
#define UHD_LOGGER_TRACE(component) _UHD_LOG_INTERNAL(component, uhd::log::trace)
#define UHD_LOGGER_DEBUG(component) _UHD_LOG_INTERNAL(component, uhd::log::debug)
#define UHD_LOGGER_INFO(component) _UHD_LOG_INTERNAL(component, uhd::log::info)
#define UHD_LOGGER_WARNING(component) _UHD_LOG_INTERNAL(component, uhd::log::warning)
#define UHD_LOGGER_ERROR(component) _UHD_LOG_INTERNAL(component, uhd::log::error)
#define UHD_LOGGER_FATAL(component) _UHD_LOG_INTERNAL(component, uhd::log::fatal)


#if defined(__GNUG__)
//! Helpful debug tool to print site info
#    define UHD_HERE()            \
        UHD_LOGGER_DEBUG("DEBUG") \
            << __FILE__ << ":" << __LINE__ << " (" << UHD_PRETTY_FUNCTION << ")";
#else
//! Helpful debug tool to print site info
#    define UHD_HERE() UHD_LOGGER_DEBUG("DEBUG") << __FILE__ << ":" << __LINE__;
#endif

//! Helpful debug tool to print a variable
#define UHD_VAR(var) UHD_LOGGER_DEBUG("DEBUG") << #var << " = " << var;

//! Helpful debug tool to print a variable in hex
#define UHD_HEX(var)                                                              \
    UHD_LOGGER_DEBUG("DEBUG") << #var << " = 0x" << std::hex << std::setfill('0') \
                              << std::setw(8) << var << std::dec;

//! \cond
namespace uhd {
namespace _log {

//! Fastpath logging
void UHD_API log_fastpath(const std::string&);

//! Internal logging object (called by UHD_LOG* macros)
class UHD_API log
{
public:
    log(const uhd::log::severity_level verbosity,
        const std::string& file,
        const unsigned int line,
        const std::string& component,
        const std::thread::id thread_id);

    ~log(void);

// Macro for overloading insertion operators to avoid costly
// conversion of types if not logging.
#define INSERTION_OVERLOAD(x) \
    log& operator<<(x)        \
    {                         \
        if (_log_it) {        \
            _ss << val;       \
        }                     \
        return *this;         \
    }

    // General insertion overload
    template <typename T>
    INSERTION_OVERLOAD(T val)

    // Insertion overloads for std::ostream manipulators
    INSERTION_OVERLOAD(std::ostream& (*val)(std::ostream&))
        INSERTION_OVERLOAD(std::ios& (*val)(std::ios&))
            INSERTION_OVERLOAD(std::ios_base& (*val)(std::ios_base&))

                private : uhd::log::logging_info _log_info;
    std::ostringstream _ss;
    const bool _log_it;
};

} // namespace _log
//! \endcond
} /* namespace uhd */
