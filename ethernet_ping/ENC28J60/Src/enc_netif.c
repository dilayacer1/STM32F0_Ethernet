/*
 * enc_netif.c
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 */

#include "enc_netif.h"

uint8_t mac_addr[6] = {0x02, 0x00, 0x00, 0x11, 0x22, 0x33};


err_t enc28j60_low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    uint16_t len = 0;
    uint8_t tx_buf[600];

    // pbuf zincirini düzleştir
    for (q = p; q != NULL; q = q->next)
    {
        memcpy(&tx_buf[len], q->payload, q->len);
        len += q->len;
    }

    ENC28J60_SendPacket(tx_buf, len);
    return ERR_OK;
}

// netif init
err_t enc28j60_netif_init(struct netif *netif)
{
    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, mac_addr, 6);
    netif->mtu = 500; // RAM kısıtlı, küçük tuttuk
    netif->flags = NETIF_FLAG_BROADCAST |
                   NETIF_FLAG_ETHARP |
                   NETIF_FLAG_LINK_UP;
    netif->output     = etharp_output;
    netif->linkoutput = enc28j60_low_level_output;
    return ERR_OK;
}
