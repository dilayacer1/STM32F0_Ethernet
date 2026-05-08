/*
 * http_server.c
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 */

#include "http_server.h"


extern uint16_t button_count;


err_t http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)  //buraya girmiyor buffer arttırılmalı
{
    if (p == NULL)
    {
        tcp_close(pcb);
        return ERR_OK;
    }

    // GET isteği mi?
    if (strncmp((char*)p->payload, "GET", 3) == 0)
    {
        char html[200];
        snprintf(html, sizeof(html),
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "<html><body>"
            "<h1>Buton: %d</h1>"
            "<meta http-equiv='refresh' content='1'>"
            "</body></html>",
            button_count);

        tcp_write(pcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
        tcp_output(pcb);
        tcp_close(pcb);
    }

    pbuf_free(p);
    return ERR_OK;
}

err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	   if (err != ERR_OK || newpcb == NULL)
	        return ERR_VAL;

	    tcp_recv(newpcb, http_recv);
	    tcp_nagle_disable(newpcb); // küçük paketleri bekletme
	    return ERR_OK;
}
