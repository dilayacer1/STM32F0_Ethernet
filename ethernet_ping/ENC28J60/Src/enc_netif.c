/*
 * enc_netif.c
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 *
 * ENC28J60 düşük seviyeli sürücüsünü lwIP ağ yığınına bağlayan adaptör katmanı.
 *
 * Mimari içindeki yeri:
 *   lwIP (TCP/IP yığını)
 *       ↕  etharp_output / enc28j60_low_level_output
 *   enc_netif.c  ← bu dosya
 *       ↕  ENC28J60_SendPacket / ENC28J60_PacketReceive
 *   enc28j60.c (SPI sürücüsü)
 *       ↕  SPI1 + PB6 (CS)
 *   ENC28J60 donanımı
 */

#include "enc_netif.h"

/*
 * Cihazın yerel MAC adresi.
 * İlk bayt 0x02: locally administered (üretici tarafından atanmamış) ve unicast.
 * Bu adres hem ENC28J60_Init hem de lwIP netif yapısına kopyalanır.
 */
uint8_t mac_addr[6] = {0x02, 0x00, 0x00, 0x11, 0x22, 0x33};

/*
 * enc28j60_low_level_output
 * lwIP'nin TX yolu bu callback'i çağırır: pbuf zincirini tek bir düz tampona
 * kopyalar ve ENC28J60_SendPacket aracılığıyla SPI üzerinden gönderir.
 *
 * lwIP büyük paketleri birden fazla pbuf'a bölebilir (scatter-gather); for
 * döngüsü bu parçaları sırayla tx_buf'a birleştirir (gather).
 *
 *   netif : aktif ağ arayüzü (bu implementasyonda kullanılmıyor)
 *   p     : gönderilecek veriyi içeren pbuf zincirinin başı
 * Dönüş: ERR_OK
 */
err_t enc28j60_low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    uint16_t len = 0;
    uint8_t tx_buf[600]; /* ENC28J60 TX tamponu 0x0C00–0x1FFF; 600 byte MTU'ya yeter */

    /* pbuf zincirini doğrusal tx_buf'a kopyala (gather) */
    for (q = p; q != NULL; q = q->next)
    {
        memcpy(&tx_buf[len], q->payload, q->len);
        len += q->len;
    }

    /* Birleştirilmiş çerçeveyi ENC28J60 TX tamponuna yaz ve ilet */
    ENC28J60_SendPacket(tx_buf, len);
    return ERR_OK;
}

/*
 * enc28j60_netif_init
 * lwIP'nin netif_add() fonksiyonu tarafından başlatma sırasında çağrılır.
 * netif yapısının donanım adresi (hwaddr), MTU ve callback alanlarını doldurur.
 *
 * Yapılandırma notları:
 *   - MTU 500 bayt: STM32F0'ın kısıtlı RAM'i nedeniyle standart 1500 yerine düşük tutuldu.
 *   - NETIF_FLAG_LINK_UP: kablo takılı kabul edilir; dinamik bağlantı algılama yok.
 *   - netif->output      = etharp_output   → lwIP ARP çözümlemesini bu fonksiyon yapar.
 *   - netif->linkoutput  = enc28j60_low_level_output → ham çerçeve gönderimi.
 *
 *   netif : yapılandırılacak lwIP ağ arayüzü
 * Dönüş: ERR_OK
 */
err_t enc28j60_netif_init(struct netif *netif)
{
    netif->hwaddr_len = 6;                      /* Ethernet MAC adresi uzunluğu */
    memcpy(netif->hwaddr, mac_addr, 6);         /* MAC adresini netif yapısına kopyala */
    netif->mtu = 500;                           /* Maksimum iletim birimi (RAM kısıtlı) */
    netif->flags = NETIF_FLAG_BROADCAST |       /* Broadcast paketlere izin ver  */
                   NETIF_FLAG_ETHARP    |       /* ARP desteği etkin             */
                   NETIF_FLAG_LINK_UP;          /* Bağlantı her zaman açık kabul */

    netif->output     = etharp_output;               /* IP → ARP → linkoutput zinciri */
    netif->linkoutput = enc28j60_low_level_output;   /* Ham Ethernet çerçevesi TX     */
    return ERR_OK;
}
