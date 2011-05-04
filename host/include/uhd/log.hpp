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

#ifndef INCLUDED_UHD_LOG_HPP
#define INCLUDED_UHD_LOG_HPP

#include <uhd/config.hpp>
#include <boost/current_function.hpp>
#include <ostream>
#include <string>

//! uhd logger macro with default verbosity
#define UHD_LOG \
    uhd::_log::log(uhd::_log::regularly, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION)

//! uhd logger macro with configurable verbosity
#define UHD_LOGV(verbosity) \
    uhd::_log::log(uhd::_log::verbosity, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION)

namespace uhd{ namespace _log{

    //! Verbosity levels for the logger
    enum verbosity_t{
        always      = 1,
        often       = 2,
        regularly   = 3,
        rarely      = 4,
        very_rarely = 5,
    };

    //! Internal logging object (called by UHD_LOG macros)
    struct UHD_API log{
        log(
            const verbosity_t verbosity,
            const std::string &file,
            const unsigned int line,
            const std::string &function
        );
        ~log(void);

        std::ostream &get(void);

        template <typename T> std::ostream &operator<<(const T &x){
            return get() << x;
        }
    };

}} //namespace uhd::_log

#endif /* INCLUDED_UHD_LOG_HPP */
