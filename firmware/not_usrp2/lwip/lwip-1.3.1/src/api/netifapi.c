/**
 * @file
 * Network Interface Sequential API module
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 */

#include "lwip/opt.h"

#if LWIP_NETIF_API /* don't build if not configured for use in lwipopts.h */

#include "lwip/netifapi.h"
#include "lwip/tcpip.h"

/**
 * Call netif_add() inside the tcpip_thread context.
 */
void
do_netifapi_netif_add( struct netifapi_msg_msg *msg)
{
  if (!netif_add( msg->netif,
                  msg->msg.add.ipaddr,
                  msg->msg.add.netmask,
                  msg->msg.add.gw,
                  msg->msg.add.state,
                  msg->msg.add.init,
                  msg->msg.add.input)) {
    msg->err = ERR_IF;
  } else {
    msg->err = ERR_OK;
  }
  TCPIP_NETIFAPI_ACK(msg);
}

/**
 * Call the "errtfunc" (or the "voidfunc" if "errtfunc" is NULL) inside the
 * tcpip_thread context.
 */
void
do_netifapi_netif_common( struct netifapi_msg_msg *msg)
{
  if (msg->msg.common.errtfunc!=NULL) {
    msg->err =
    msg->msg.common.errtfunc(msg->netif);
  } else {
    msg->err = ERR_OK;
    msg->msg.common.voidfunc(msg->netif);
  }
  TCPIP_NETIFAPI_ACK(msg);
}

/**
 * Call netif_add() in a thread-safe way by running that function inside the
 * tcpip_thread context.
 *
 * @note for params @see netif_add()
 */
err_t
netifapi_netif_add(struct netif *netif,
                   struct ip_addr *ipaddr,
                   struct ip_addr *netmask,
                   struct ip_addr *gw,
                   void *state,
                   err_t (* init)(struct netif *netif),
                   err_t (* input)(struct pbuf *p, struct netif *netif))
{
  struct netifapi_msg msg;
  msg.function = do_netifapi_netif_add;
  msg.msg.netif = netif;
  msg.msg.msg.add.ipaddr  = ipaddr;
  msg.msg.msg.add.netmask = netmask;
  msg.msg.msg.add.gw      = gw;
  msg.msg.msg.add.state   = state;
  msg.msg.msg.add.init    = init;
  msg.msg.msg.add.input   = input;
  TCPIP_NETIFAPI(&msg);
  return msg.msg.err;
}

/**
 * call the "errtfunc" (or the "voidfunc" if "errtfunc" is NULL) in a thread-safe
 * way by running that function inside the tcpip_thread context.
 *
 * @note use only for functions where there is only "netif" parameter.
 */
err_t
netifapi_netif_common( struct netif *netif,
                       void  (* voidfunc)(struct netif *netif),
                       err_t (* errtfunc)(struct netif *netif) )
{
  struct netifapi_msg msg;
  msg.function = do_netifapi_netif_common;
  msg.msg.netif = netif;
  msg.msg.msg.common.voidfunc = voidfunc;
  msg.msg.msg.common.errtfunc = errtfunc;
  TCPIP_NETIFAPI(&msg);
  return msg.msg.err;
}

#endif /* LWIP_NETIF_API */
