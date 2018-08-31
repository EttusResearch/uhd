//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef _UHD_DPDK_FOPS_H_
#define _UHD_DPDK_FOPS_H_

#include "uhd_dpdk_ctx.h"

int _uhd_dpdk_config_req_compl(struct uhd_dpdk_config_req *req, int retval);

int _uhd_dpdk_sock_setup(struct uhd_dpdk_config_req *req);
int _uhd_dpdk_sock_release(struct uhd_dpdk_config_req *req);

/*
 * Get key for RX table corresponding to this socket
 *
 * This is primarily used to get access to the waiter entry
 */
int _uhd_dpdk_sock_rx_key(struct uhd_dpdk_socket *sock,
                          struct uhd_dpdk_ipv4_5tuple *key);
#endif /* _UHD_DPDK_FOPS_H_ */
