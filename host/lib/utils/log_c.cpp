/*
 * Copyright 2017 Ettus Research (National Instruments Corp)
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
