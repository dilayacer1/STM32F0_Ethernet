/*
 * http_server.c
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 *
 * lwIP raw TCP API kullanan minimal HTTP/1.0 sunucu implementasyonu.
 *
 * Çalışma akışı:
 *   1. main.c'de tcp_new() + tcp_bind(port 80) + tcp_listen() + tcp_accept(http_accept)
 *   2. İstemci bağlandığında lwIP → http_accept() → tcp_recv(http_recv) kaydeder
 *   3. İstemciden GET isteği geldiğinde lwIP → http_recv() → HTML yanıt üretir
 *
 * Kısıtlamalar:
 *   - Tampon boyutu 200 byte: STM32F0 RAM kısıtı nedeniyle küçük tutulmuştur.
 *     Büyük HTML sayfaları için tcp_write çok parçalı (chunked) gönderim gerekir.
 *   - HTTP/1.0 kullanılır: her istek için bağlantı açılır, yanıt gönderilir, kapanır.
 */

#include "http_server.h"

/* button_count: main.c'de GPIO interrupt ile güncellenen buton basma sayacı */
extern uint16_t button_count;

/*
 * http_recv
 * lwIP TCP alma callback'i — bağlantıda yeni veri geldiğinde otomatik çağrılır.
 *
 * p == NULL: bağlantı karşı tarafça kapatılmış; tcp_close ile yerel tarafı kapat.
 * GET isteği: HTTP/1.0 200 OK başlığı + buton sayacını içeren HTML oluştur ve gönder.
 * Nagle devre dışı (http_accept'te ayarlandı): tcp_output çağrısı anlık gönderimi zorlar.
 *
 * NOT: html tamponu 200 byte; URL veya sayfa içeriği büyürse taşma riski var.
 *      Daha büyük yanıtlar için snprintf dönüş değeri kontrol edilmeli.
 */
err_t http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    if (p == NULL)
    {
        /* Karşı taraf bağlantıyı kapattı — yerel TCP oturumunu temizle */
        tcp_close(pcb);
        return ERR_OK;
    }

    /* İlk 3 baytı kontrol ederek HTTP GET isteği olup olmadığını belirle */
    if (strncmp((char*)p->payload, "GET", 3) == 0)
    {
        char html[200];
        snprintf(html, sizeof(html),
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "<html><body>"
            "<h1>Buton: %d</h1>"
            /* Meta refresh: tarayıcı sayfayı her 1 saniyede otomatik yeniler */
            "<meta http-equiv='refresh' content='1'>"
            "</body></html>",
            button_count);

        /* TCP gönderme kuyruğuna yaz; TCP_WRITE_FLAG_COPY: lwIP kendi kopyasını tutar */
        tcp_write(pcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
        tcp_output(pcb);  /* Nagle beklemeden tamponu hemen gönder */
        tcp_close(pcb);   /* HTTP/1.0: yanıt sonrası bağlantıyı kapat */
    }

    /* pbuf'u serbest bırak — bellek sızıntısını önle */
    pbuf_free(p);
    return ERR_OK;
}

/*
 * http_accept
 * lwIP TCP bağlantı kabul callback'i — istemci 80. porta bağlandığında çağrılır.
 *
 * Yeni bağlantı (newpcb) için:
 *   - http_recv callback'ini kaydet
 *   - Nagle algoritmasını devre dışı bırak: küçük HTTP yanıtları gecikme olmadan gönderilir
 *     (Nagle etkin olsaydı lwIP yanıtı biriktirip ACK bekliyerek geciktirebilirdi)
 */
err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    if (err != ERR_OK || newpcb == NULL)
        return ERR_VAL; /* Geçersiz bağlantı — reddet */

    /* Gelen veriler için alma callback'ini kaydet */
    tcp_recv(newpcb, http_recv);

    /* tcp_nagle_disable: küçük paketleri biriktirmeden anında ilet */
    tcp_nagle_disable(newpcb);
    return ERR_OK;
}
