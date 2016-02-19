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

#include <cron.h>
#include <wb_soft_reg.h>
#include <u3_net_stack.h>
#include <trace.h>
#include "../../../host/lib/usrp/n230/n230_fw_defs.h"
#include "../../../host/lib/usrp/n230/n230_fw_host_iface.h"
#include "n230_eth_handlers.h"
#include "n230_init.h"

//The version hash should come from a cmake build variable
//If it doesn't then the build system does not support the feature
//so just default to 0xFFFFFFFF
#ifndef UHD_VERSION_HASH
#define UHD_VERSION_HASH 0xFFFFFFFF
#endif

//TODO: This is just for initial debugging.
static soft_reg_t g_led_register;

//Shared memory
static n230_host_shared_mem_t g_host_shared_mem;

//Functions
static void n230_handle_claim();

/***********************************************************************
 * Main loop runs all the handlers
 **********************************************************************/
int main(void)
{
    //Initialize host shared mem
    g_host_shared_mem.data.fw_compat_num    = N230_FW_COMPAT_NUM;
    g_host_shared_mem.data.fw_version_hash  = UHD_VERSION_HASH;

    //Main initialization function
    n230_init();

    //Initialize UDP Handlers
    n230_register_udp_fw_comms_handler(&g_host_shared_mem);
    n230_register_udp_prog_framer();
    n230_register_flash_comms_handler();

    initialize_writeonly_soft_reg(&g_led_register, SR_ADDR(WB_SBRB_BASE, SR_ZPU_LEDS));

    uint32_t heart_beat = 0;
    while(true)
    {
        //TODO: This is just for initial debugging. Once the firmware
        //is somewhat stable we should delete this cron job
        if (cron_job_run_due(PER_SECOND_CRON_JOBID)) {
            //Everything in this block runs approx once per second
            if (heart_beat % 10 == 0) {
                UHD_FW_TRACE_FSTR(INFO, "0.1Hz Heartbeat (%u)", heart_beat);
            }
            heart_beat++;
        }

        if (cron_job_run_due(PER_MILLISEC_CRON_JOBID)) {
            //Everything in this block runs approx once per millisecond
            n230_handle_claim();
            n230_update_link_act_state(&g_led_register);
        }

        //run the network stack - poll and handle
        u3_net_stack_handle_one();
    }
    return 0;
}

// Watchdog timer for claimer
static void n230_handle_claim()
{
    static uint32_t last_time = 0;
    static size_t timeout = 0;

    if (g_host_shared_mem.data.claim_time == 0) {
        //If time is 0 if the claim was forfeit
        g_host_shared_mem.data.claim_status = 0;
    } else if (last_time != g_host_shared_mem.data.claim_time) {
        //If the time changes, reset timeout
        g_host_shared_mem.data.claim_status = 1;
        timeout = 0;
    } else {
        //Otherwise increment for timeout
        timeout++;
    }

    //Always stash the last seen time
    last_time = g_host_shared_mem.data.claim_time;

    //Timeout logic
    if (timeout > N230_CLAIMER_TIMEOUT_IN_MS) {
        g_host_shared_mem.data.claim_time = 0;
    }
}

