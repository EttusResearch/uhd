/*
 * Copyright 2017 Ettus Research (National Instruments Corp.)
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

#ifndef INCLUDED_UHD_UTILS_LOG_H
#define INCLUDED_UHD_UTILS_LOG_H

#include <uhd/config.h>

typedef enum {
    UHD_LOG_LEVEL_TRACE,
    UHD_LOG_LEVEL_DEBUG,
    UHD_LOG_LEVEL_INFO,
    UHD_LOG_LEVEL_WARNING,
    UHD_LOG_LEVEL_ERROR,
    UHD_LOG_LEVEL_FATAL
} uhd_log_severity_level_t;

#ifdef __cplusplus
extern "C" {
#endif

    void UHD_API _uhd_log(
            const uhd_log_severity_level_t log_level,
            const char *filename,
            const int lineno,
            const char *comp,
            const char *format,
            ...
    );

#ifdef __cplusplus
};
#endif


#ifndef __cplusplus
// macro-style logging (compile-time determined)
#if UHD_LOG_MIN_LEVEL < 1
#define UHD_LOG_TRACE(component, ...) \
    _uhd_log(UHD_LOG_LEVEL_TRACE, __FILE__, __LINE__, component, __VA_ARGS__);
#else
#define UHD_LOG_TRACE(component, ...)
#endif

#if UHD_LOG_MIN_LEVEL < 2
#define UHD_LOG_DEBUG(component, ...) \
    _uhd_log(UHD_LOG_LEVEL_DEBUG, __FILE__, __LINE__, component, __VA_ARGS__);
#else
#define UHD_LOG_DEBUG(component, ...)
#endif

#if UHD_LOG_MIN_LEVEL < 3
#define UHD_LOG_INFO(component, ...) \
    _uhd_log(UHD_LOG_LEVEL_INFO, __FILE__, __LINE__, component, __VA_ARGS__);
#else
#define UHD_LOG_INFO(component, ...)
#endif

#if UHD_LOG_MIN_LEVEL < 4
#define UHD_LOG_WARNING(component, ...) \
    _uhd_log(UHD_LOG_LEVEL_WARNING, __FILE__, __LINE__, component, __VA_ARGS__);
#else
#define UHD_LOG_WARNING(component, ...)
#endif

#if UHD_LOG_MIN_LEVEL < 5
#define UHD_LOG_ERROR(component, ...) \
    _uhd_log(UHD_LOG_LEVEL_ERROR, __FILE__, __LINE__, component, __VA_ARGS__);
#else
#define UHD_LOG_ERROR(component, ...)
#endif

#if UHD_LOG_MIN_LEVEL < 6
#define UHD_LOG_FATAL(component, ...) \
    _uhd_log(UHD_LOG_LEVEL_FATAL, __FILE__, __LINE__, component, __VA_ARGS__);
#else
#define UHD_LOG_FATAL(component, ...)
#endif

#endif /* #ifndef __cplusplus */

#endif /* INCLUDED_UHD_UTILS_LOG_H */
