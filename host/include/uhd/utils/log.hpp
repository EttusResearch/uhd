//
// Copyright 2011 Ettus Research LLC
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

#ifndef INCLUDED_UHD_UTILS_LOG_HPP
#define INCLUDED_UHD_UTILS_LOG_HPP

#include <uhd/config.hpp>
#include <uhd/utils/pimpl.hpp>
#include <boost/current_function.hpp>
#include <boost/format.hpp>
#include <ostream>
#include <string>
#include <sstream>

/*! \file log.hpp
 * The UHD logging facility.
 *
 * The logger enables UHD library code to easily log events into a file.
 * Log entries are time-stamped and stored with file, line, and function.
 * Each call to the UHD_LOG macros is synchronous and thread-safe.
 *
 * The log file can be found in the path <temp-directory>/uhd.log,
 * where <temp-directory> is the user or system's temporary directory.
 * To override <temp-directory>, set the UHD_TEMP_PATH environment variable.
 *
 * All log messages with verbosity greater than or equal to the log level
 * (in other words, as often or less often than the current log level)
 * are recorded into the log file. All other messages are sent to null.
 *
 * The default log level is "never", but can be overridden:
 *  - at compile time by setting the pre-processor define UHD_LOG_LEVEL.
 *  - at runtime by setting the environment variable UHD_LOG_LEVEL.
 *
 * UHD_LOG_LEVEL can be the name of a verbosity enum or integer value:
 *   - Example pre-processor define: -DUHD_LOG_LEVEL=3
 *   - Example pre-processor define: -DUHD_LOG_LEVEL=regularly
 *   - Example environment variable: export UHD_LOG_LEVEL=3
 *   - Example environment variable: export UHD_LOG_LEVEL=regularly
 */

/*!
 * A UHD logger macro with configurable verbosity.
 * Usage: UHD_LOGV(very_rarely) << "the log message" << std::endl;
 */
#define UHD_LOGV(verbosity) \
    uhd::_log::log(uhd::_log::verbosity, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION)

/*!
 * A UHD logger macro with default verbosity.
 * Usage: UHD_LOG << "the log message" << std::endl;
 */
#define UHD_LOG \
    UHD_LOGV(regularly)


namespace uhd{ namespace _log{

    //! Verbosity levels for the logger
    enum verbosity_t{
        always      = 1,
        often       = 2,
        regularly   = 3,
        rarely      = 4,
        very_rarely = 5,
        never       = 6,
    };

    //! Internal logging object (called by UHD_LOG macros)
    class UHD_API log {
    public:
        log(
            const verbosity_t verbosity,
            const std::string &file,
            const unsigned int line,
            const std::string &function
        );

        ~log(void);

        // Macro for overloading insertion operators to avoid costly
        // conversion of types if not logging.
        #define INSERTION_OVERLOAD(x)   log& operator<< (x)             \
                                        {                               \
                                            if(_log_it) _ss << val;     \
                                            return *this;               \
                                        }

        // General insertion overload
        template <typename T>
        INSERTION_OVERLOAD(T val);

        // Insertion overloads for std::ostream manipulators
        INSERTION_OVERLOAD(std::ostream& (*val)(std::ostream&));
        INSERTION_OVERLOAD(std::ios& (*val)(std::ios&));
        INSERTION_OVERLOAD(std::ios_base& (*val)(std::ios_base&));

    private:
        std::ostringstream _ss;
        bool _log_it;
    };

}} //namespace uhd::_log

#endif /* INCLUDED_UHD_UTILS_LOG_HPP */
