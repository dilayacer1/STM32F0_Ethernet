/*
 * http_server.h
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 *
 * lwIP raw TCP API'si üzerine kurulu minimal HTTP/1.0 sunucu başlık dosyası.
 * Sunucu, buton sayacını gösteren tek sayfalık bir HTML yanıtı üretir.
 */

#ifndef INC_HTTP_SERVER_H_
#define INC_HTTP_SERVER_H_

#include "main.h"
#include "lwip/err.h"      /* ERR_OK, ERR_VAL gibi lwIP hata kodları      */
#include "lwip/netif.h"    /* netif yapısı                                 */
#include "lwip/pbuf.h"     /* paket tampon zinciri                         */
#include  "lwip/etharp.h"  /* ARP desteği                                  */
#include  "lwip/tcpbase.h" /* TCP temel türleri ve sabitleri               */
#include "lwip/tcp.h"      /* lwIP raw TCP API (tcp_write, tcp_close, vb.) */

/*
 * http_recv
 * TCP alma callback'i — lwIP, bağlantıda yeni veri geldiğinde çağırır.
 * Gelen veri bir HTTP GET isteği ise HTML yanıt üretir ve bağlantıyı kapatır.
 *   arg   : kullanıcı verisi (kullanılmıyor)
 *   pcb   : TCP kontrol bloğu
 *   p     : gelen veriyi içeren pbuf (NULL ise bağlantı kapandı)
 *   err   : lwIP hata kodu
 * Dönüş: ERR_OK
 */
err_t http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);

/*
 * http_accept
 * TCP bağlantı kabul callback'i — lwIP, istemci bağlandığında çağırır.
 * Yeni bağlantı için http_recv'i kayıt eder ve Nagle algoritmasını devre dışı bırakır.
 *   arg    : kullanıcı verisi (kullanılmıyor)
 *   newpcb : yeni kabul edilen TCP kontrol bloğu
 *   err    : lwIP hata kodu
 * Dönüş: ERR_OK veya ERR_VAL (geçersiz pcb)
 */
err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

#endif /* INC_HTTP_SERVER_H_ */
