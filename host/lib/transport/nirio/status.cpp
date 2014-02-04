//
// Copyright 2013 Ettus Research LLC
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


#include <uhd/transport/nirio/status.h>
#include <boost/format.hpp>

namespace uhd { namespace niusrprio {

#define NIRIO_ERR_INFO(CONST_NAME, ERR_CODE, ERR_MSG) \
    nirio_err_info(ERR_CODE, ERR_MSG),

const nirio_err_info nirio_err_info::NIRIO_ERROR_TABLE[] = {
    #include "../../../include/uhd/transport/nirio/nirio_err_template.h"
};

#undef NIRIO_ERR_INFO

const size_t nirio_err_info::NIRIO_ERROR_TABLE_SIZE = sizeof(NIRIO_ERROR_TABLE)/sizeof(*NIRIO_ERROR_TABLE);

const std::string lookup_err_msg(nirio_status code) {
    std::string error_msg = (boost::format("Unknown error. (Error code %d)") % code).str();
    for (size_t i = 0; i < nirio_err_info::NIRIO_ERROR_TABLE_SIZE; i++) {
        if (nirio_err_info::NIRIO_ERROR_TABLE[i].code == code) {
            error_msg = (boost::format("%s (Error code %d)") % nirio_err_info::NIRIO_ERROR_TABLE[i].msg % code).str();
            break;
        }
    }
    return error_msg;
}

void nirio_status_to_exception(const nirio_status& status, const std::string& message) {
    if (nirio_status_fatal(status)) {
        throw uhd::runtime_error((boost::format("%s %s") % message % lookup_err_msg(status)).str());
    }
}

}}



