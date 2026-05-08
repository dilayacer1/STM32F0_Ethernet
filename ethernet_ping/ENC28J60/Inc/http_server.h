/*
 * http_server.h
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 */

#ifndef INC_HTTP_SERVER_H_
#define INC_HTTP_SERVER_H_

#include "main.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include  "lwip/etharp.h"
#include  "lwip/tcpbase.h"
#include "lwip/tcp.h"

err_t http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
#endif /* INC_HTTP_SERVER_H_ */
