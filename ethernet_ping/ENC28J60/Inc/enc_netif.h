/*
 * enc_netif.h
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 */

#ifndef INC_ENC_NETIF_H_
#define INC_ENC_NETIF_H_

#include "main.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include  "lwip/etharp.h"

err_t enc28j60_low_level_output(struct netif *netif, struct pbuf *p);
err_t enc28j60_netif_init(struct netif *netif);

#endif /* INC_ENC_NETIF_H_ */
