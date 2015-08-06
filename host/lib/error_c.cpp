//
// Copyright 2015 Ettus Research LLC
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

#include <uhd/error.h>
#include <uhd/exception.hpp>

#define MAP_TO_ERROR(exception_type, error_type) \
    if (dynamic_cast<const uhd::exception_type*>(e)) return error_type;

uhd_error error_from_uhd_exception(const uhd::exception* e){
    MAP_TO_ERROR(index_error,           UHD_ERROR_INDEX)
    MAP_TO_ERROR(key_error,             UHD_ERROR_KEY)
    MAP_TO_ERROR(not_implemented_error, UHD_ERROR_NOT_IMPLEMENTED)
    MAP_TO_ERROR(usb_error,             UHD_ERROR_USB)
    MAP_TO_ERROR(io_error,              UHD_ERROR_IO)
    MAP_TO_ERROR(os_error,              UHD_ERROR_OS)
    MAP_TO_ERROR(assertion_error,       UHD_ERROR_ASSERTION)
    MAP_TO_ERROR(lookup_error,          UHD_ERROR_LOOKUP)
    MAP_TO_ERROR(type_error,            UHD_ERROR_TYPE)
    MAP_TO_ERROR(value_error,           UHD_ERROR_VALUE)
    MAP_TO_ERROR(runtime_error,         UHD_ERROR_RUNTIME)
    MAP_TO_ERROR(environment_error,     UHD_ERROR_ENVIRONMENT)
    MAP_TO_ERROR(system_error,          UHD_ERROR_SYSTEM)

    return UHD_ERROR_EXCEPT;
}
