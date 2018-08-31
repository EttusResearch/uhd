//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "uhd_dpdk_wait.h"

int _uhd_dpdk_waiter_wake(struct uhd_dpdk_wait_req *req,
                          struct uhd_dpdk_thread *t)
{
    int stat = pthread_mutex_trylock(&req->mutex);
    if (stat) {
        if (rte_ring_full(t->waiter_ring)) {
            RTE_LOG(ERR, USER2, "%s: Could not lock req mutex\n", __func__);
            return -ENOBUFS;
        } else {
            req->reason = UHD_DPDK_WAIT_SIMPLE;
            rte_ring_enqueue(t->waiter_ring, req);
            return -EAGAIN;
        }
    }
    stat = pthread_cond_signal(&req->cond);
    if (stat)
        RTE_LOG(ERR, USER2, "%s: Could not signal req cond\n", __func__);
    pthread_mutex_unlock(&req->mutex);
    uhd_dpdk_waiter_put(req);
    return stat;
}

struct uhd_dpdk_wait_req *uhd_dpdk_waiter_alloc(enum uhd_dpdk_wait_type reason)
{
    struct uhd_dpdk_wait_req *req;
    req = (struct uhd_dpdk_wait_req *) rte_zmalloc(NULL, sizeof(*req), 0);
    if (!req)
        return NULL;

    pthread_mutex_init(&req->mutex, NULL);
    pthread_condattr_t condattr;
    pthread_condattr_init(&condattr);
    pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
    pthread_cond_init(&req->cond, &condattr);
    rte_atomic32_set(&req->refcnt, 1);
    req->reason = reason;
    return req;
}

static inline void uhd_dpdk_waiter_prepare(struct uhd_dpdk_wait_req *req)
{
    pthread_mutex_lock(&req->mutex);
    /* Get a reference here, to be consumed by other thread (handshake) */
    uhd_dpdk_waiter_get(req);
}

static inline int uhd_dpdk_waiter_submit(struct uhd_dpdk_wait_req *req,
                                         int timeout)
{
    int retval = 0;
    if (timeout < 0) {
        retval = pthread_cond_wait(&req->cond, &req->mutex);
    } else {
        struct timespec timeout_spec;
        clock_gettime(CLOCK_MONOTONIC, &timeout_spec);
        timeout_spec.tv_sec += timeout/1000000;
        timeout_spec.tv_nsec += (timeout % 1000000)*1000;
        if (timeout_spec.tv_nsec > 1000000000) {
            timeout_spec.tv_sec++;
            timeout_spec.tv_nsec -= 1000000000;
        }
        retval = pthread_cond_timedwait(&req->cond, &req->mutex, &timeout_spec);
    }
    return retval;
}

int uhd_dpdk_waiter_wait(struct uhd_dpdk_wait_req *req, int timeout,
                         struct uhd_dpdk_thread *t)
{
    int ret;
    if (!req || !t)
        return -EINVAL;

    uhd_dpdk_waiter_prepare(req);

    ret = rte_ring_enqueue(t->waiter_ring, req);
    if (ret) {
        uhd_dpdk_waiter_put(req);
        pthread_mutex_unlock(&req->mutex);
        return ret;
    }

    uhd_dpdk_waiter_submit(req, timeout);
    pthread_mutex_unlock(&req->mutex);
    return 0;
}

int uhd_dpdk_config_req_submit(struct uhd_dpdk_config_req *req,
                               int timeout, struct uhd_dpdk_thread *t)
{
    int ret;
    if (!req || !t)
        return -EINVAL;

    uhd_dpdk_waiter_prepare(req->waiter);

    ret = rte_ring_enqueue(t->sock_req_ring, req);
    if (ret) {
        uhd_dpdk_waiter_put(req->waiter);
        pthread_mutex_unlock(&req->waiter->mutex);
        return ret;
    }

    uhd_dpdk_waiter_submit(req->waiter, timeout);
    pthread_mutex_unlock(&req->waiter->mutex);
    return 0;
}
