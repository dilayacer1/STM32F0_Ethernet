/*
 * enc_netif.h
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 *
 * ENC28J60 sürücüsünü lwIP ağ yığınına bağlayan netif adaptör başlık dosyası.
 * lwIP, donanımla doğrudan iletişim kurmak için bu dosyadaki callback fonksiyonlarını kullanır.
 */

#ifndef INC_ENC_NETIF_H_
#define INC_ENC_NETIF_H_

#include "main.h"
#include "lwip/err.h"     /* lwIP hata kodları (ERR_OK, ERR_VAL, vb.)        */
#include "lwip/netif.h"   /* netif yapısı ve ilgili bayraklar                 */
#include "lwip/pbuf.h"    /* paket tampon zinciri (pbuf) tanımları            */
#include  "lwip/etharp.h" /* ARP çözümlemesi için etharp_output callback'i   */

/*
 * enc28j60_low_level_output
 * lwIP tarafından çağrılan TX callback'i.
 * pbuf zincirini düz bir bayt dizisine çevirir ve ENC28J60_SendPacket ile gönderir.
 *   netif : işlemi yapan ağ arayüzü (kullanılmıyor, imza gereği)
 *   p     : gönderilecek Ethernet çerçevesini içeren pbuf zinciri
 * Dönüş: ERR_OK — her zaman başarılı kabul edilir
 */
err_t enc28j60_low_level_output(struct netif *netif, struct pbuf *p);

/*
 * enc28j60_netif_init
 * lwIP netif başlatma callback'i — netif_add() tarafından çağrılır.
 * Donanım adresini (MAC), MTU'yu ve çıkış callback'lerini netif yapısına yazar.
 *   netif : yapılandırılacak lwIP ağ arayüzü
 * Dönüş: ERR_OK
 */
err_t enc28j60_netif_init(struct netif *netif);

#endif /* INC_ENC_NETIF_H_ */
