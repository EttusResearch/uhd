/*
 * Copyright 2017 Ettus Research (National Instruments Corp)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <uhd/utils/log.h>
#include <uhd/utils/log.hpp>
#include <stdarg.h>


void UHD_API _uhd_log(
        const uhd_log_severity_level_t log_level,
        const char *filename,
        const int lineno,
        const char *component,
        const char *format,
        ...
) {
    int size = 0;
    char *c_str = NULL;
    va_list ap;

    /* figure out size */
    va_start(ap, format);
    size = vsnprintf(c_str, size, format, ap);
    va_end(ap);

    if (size < 0) {
        return;
    }

    /* trailing '\0' */
    size++;
    c_str = (char *)malloc(size);
    if (!c_str) {
        return;
    }

    va_start(ap, format);
    size = vsnprintf(c_str, size, format, ap);
    if (size < 0) {
        goto out_free;
    }
    va_end(ap);

    try {
        uhd::_log::log(
            static_cast<uhd::log::severity_level>(log_level),
            filename,
            unsigned(lineno),
            component,
            boost::this_thread::get_id()
        ) << c_str;
    } catch (...) {}

out_free:
    free(c_str);
    return;
}
