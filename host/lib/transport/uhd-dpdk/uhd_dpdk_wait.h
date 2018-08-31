//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef _UHD_DPDK_WAIT_H_
#define _UHD_DPDK_WAIT_H_

#include "uhd_dpdk_ctx.h"
#include <rte_malloc.h>

enum uhd_dpdk_wait_type {
    UHD_DPDK_WAIT_SIMPLE,
    UHD_DPDK_WAIT_RX,
    UHD_DPDK_WAIT_TX_BUF,
    UHD_DPDK_WAIT_TYPE_COUNT
};

struct uhd_dpdk_wait_req {
    enum uhd_dpdk_wait_type reason;
    struct uhd_dpdk_socket *sock;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    rte_atomic32_t refcnt; /* free resources only when refcnt = 0 */
};

static inline void uhd_dpdk_waiter_put(struct uhd_dpdk_wait_req *req)
{
    if (rte_atomic32_dec_and_test(&req->refcnt)) {
        rte_free(req);
    }
}

static inline void uhd_dpdk_waiter_get(struct uhd_dpdk_wait_req *req)
{
    rte_atomic32_inc(&req->refcnt);
}

/*
 * Attempt to wake thread
 * Re-enqueue waiter to thread's waiter_queue if fail
 */
int _uhd_dpdk_waiter_wake(struct uhd_dpdk_wait_req *req,
                          struct uhd_dpdk_thread *t);

/*
 * Allocates wait request and sets refcnt to 1
 */
struct uhd_dpdk_wait_req *uhd_dpdk_waiter_alloc(enum uhd_dpdk_wait_type reason);

/*
 * Block and send wait request to thread t
 */
int uhd_dpdk_waiter_wait(struct uhd_dpdk_wait_req *req, int timeout,
                         struct uhd_dpdk_thread *t);

/*
 * Block and submit config request to thread t
 */
int uhd_dpdk_config_req_submit(struct uhd_dpdk_config_req *req,
                               int timeout, struct uhd_dpdk_thread *t);
#endif /* _UHD_DPDK_WAIT_H_ */
