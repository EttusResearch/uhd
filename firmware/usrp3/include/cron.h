//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_CRON_H
#define INCLUDED_CRON_H

#include <stdint.h>
#include <stdbool.h>

#define CRON_MAX_JOBS 4

typedef enum {
    SEC = 1, MILLISEC = 1000, MICROSEC = 1000000
} cron_time_unit_t;

typedef uint32_t (*cron_counter_fetcher_t)();

/*!
 * \brief Initialize cron subsystem with a mechanism to fetch a counter and its frequency
 */
void cron_init(const cron_counter_fetcher_t fetch_counter, uint32_t counter_freq);

/*!
 * \brief Get the hardware tick count
 */
uint32_t cron_get_ticks();

/*!
 * \brief Get the time elapsed between start and stop in the specified units
 */
uint32_t get_elapsed_time(uint32_t start_ticks, uint32_t stop_ticks, cron_time_unit_t unit);

/*!
 * \brief Sleep (spinloop) for about 'ticks' counter ticks
 * Use only if simulating, _very_ short delay
 */
void sleep_ticks(uint32_t ticks);

/*!
 * \brief Sleep (spinloop) for about 'duration' microseconds
 * Use only if simulating, _very_ short delay
 */
void sleep_us(uint32_t duration);

/*!
 * \brief Sleep (spinloop) for about 'duration' milliseconds
 * Use only if simulating, _very_ short delay
 */
void sleep_ms(uint32_t duration);

/*!
 * \brief Initialize a unique cron job with 'job_id' and interval 'interval_ms'
 */
void cron_job_init(uint32_t job_id, uint32_t interval_ms);

/*!
 * \brief Check if cron job with 'job_id' is due for execution
 */
bool cron_job_run_due(uint32_t job_id);

#endif /* INCLUDED_CRON_H */
