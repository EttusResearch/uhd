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

#include "cron.h"

//Counter specific
static cron_counter_fetcher_t  cron_fetch_counter;
static uint32_t                cron_counter_freq;

//Cron job specific
typedef struct {
    uint32_t tick_interval;
    uint32_t last_tick_count;
} cron_job_t;

static cron_job_t  cron_job_table[CRON_MAX_JOBS];

void cron_init(const cron_counter_fetcher_t fetch_counter, uint32_t counter_freq)
{
    cron_fetch_counter = fetch_counter;
    cron_counter_freq = counter_freq;

    for (int i = 0; i < CRON_MAX_JOBS; i++) {
        cron_job_table[i].tick_interval = 0;
    }
}

uint32_t cron_get_ticks()
{
    return cron_fetch_counter();
}

uint32_t get_elapsed_time(uint32_t start_ticks, uint32_t stop_ticks, cron_time_unit_t unit)
{
    return ((stop_ticks - start_ticks) / cron_counter_freq) * ((uint32_t)unit);
}

void sleep_ticks(uint32_t ticks)
{
    if (ticks == 0) return; //Handle the 0 delay case quickly

    const uint32_t ticks_begin = cron_fetch_counter();
    while(cron_fetch_counter() - ticks_begin < ticks) {
      /*NOP: Spinloop*/
    }
}

void sleep_us(uint32_t duration)
{
    sleep_ticks((duration * (cron_counter_freq/1000000)));
}

void sleep_ms(uint32_t duration)
{
    sleep_ticks((duration * (cron_counter_freq/1000)));
}

void cron_job_init(uint32_t job_id, uint32_t interval_ms)
{
    cron_job_table[job_id].tick_interval = (interval_ms * (cron_counter_freq/1000));
    cron_job_table[job_id].last_tick_count = 0;
}

bool cron_job_run_due(uint32_t job_id)
{
    uint32_t new_tick_count = cron_fetch_counter();
    bool run_job = (new_tick_count - cron_job_table[job_id].last_tick_count) >=
        cron_job_table[job_id].tick_interval;

    if (run_job) {
        //If the job is due to run, update the tick count for the next run
        //The assumption here is that the caller will actually run their job
        //when the return value is true. If not, the caller just missed this
        //iteration and will have to option to run the job in the next pass through.
        cron_job_table[job_id].last_tick_count = new_tick_count;
    }
    return run_job;
}
