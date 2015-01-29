//
// Copyright 2013-2014 Ettus Research LLC
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


#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_STATUS_H
#define INCLUDED_UHD_TRANSPORT_NIRIO_STATUS_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <uhd/config.hpp>
#include <uhd/exception.hpp>

#define ENABLE_EXTENDED_ERROR_INFO false

typedef int32_t nirio_status;

namespace uhd { namespace niusrprio {
struct nirio_err_info {
    nirio_err_info(nirio_status arg_code, const char* arg_msg): code(arg_code), msg(arg_msg) {}

    nirio_status code;
    const char* msg;

    static const nirio_err_info NIRIO_ERROR_TABLE[];
    static const size_t NIRIO_ERROR_TABLE_SIZE;
};

UHD_API const std::string lookup_err_msg(nirio_status code);

UHD_API void nirio_status_to_exception(const nirio_status& status, const std::string& message);
}}

#define nirio_status_fatal(status) ((status) < 0)
#define nirio_status_not_fatal(status) ((status) >= 0)

#define nirio_status_chain(func, status) 	\
	if (nirio_status_not_fatal(status)) {	\
		status = (func);					\
		if (ENABLE_EXTENDED_ERROR_INFO && nirio_status_fatal(status)) {	\
			fprintf(stderr,"ERROR: The following function call returned status code %d\n%s\n%s:%d\n",status,#func,__FILE__,__LINE__);	\
		}									\
	}										\


#define NIRIO_ERR_INFO(CONST_NAME, ERR_CODE, ERR_MSG) \
    static const nirio_status CONST_NAME = ERR_CODE;
#include "nirio_err_template.h"
#undef NIRIO_ERR_INFO

#endif /* INCLUDED_UHD_TRANSPORT_NIRIO_STATUS_H */
