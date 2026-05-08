/*
 * enc28j60.h
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 *
 * ENC28J60 SPI Ethernet denetleyicisi için düşük seviyeli sürücü başlık dosyası.
 * Dahili bellek haritası sabitleri, PHY register adresleri, Chip Select makroları
 * ve tüm sürücü fonksiyonlarının bildirimleri burada tanımlanır.
 */

#ifndef INC_ENC28J60_H_
#define INC_ENC28J60_H_

#include "main.h"
#include "enc_netif.h"

/* ----- Dahili SRAM bellek haritası -----
 * ENC28J60'ın 8 KB SRAM'i RX ve TX tamponlarına bölünmüştür.
 * RX: 0x0000 – 0x0BFF (3072 byte) — gelen paketleri döngüsel tampon olarak tutar.
 * TX: 0x0C00 – 0x1FFF —  gönderilecek tek paket buraya yazılır. */
#define RXSTART  0x0000   /* RX dairesel tamponunun başlangıç adresi */
#define RXEND    0x0BFF   /* RX dairesel tamponunun bitiş adresi     */
#define TXSTART  0x0C00   /* TX tamponunun başlangıç adresi          */

/* ----- PHY register adresleri -----
 * MII (Media Independent Interface) aracılığıyla erişilir;
 * doğrudan SPI opcode'u ile değil, MIREGADR/MICMD/MIWR/MIRD yazmaçları üzerinden okunup yazılır. */
#define PHCON1   0x00   /* PHY Kontrol Kaydı 1 — duplex, loopback, reset       */
#define PHSTAT1  0x01   /* PHY Durum Kaydı 1  — bağlantı durumu, yetenekler   */
#define PHID1    0x02   /* PHY Kimlik Kaydı   — üretici OUI (yüksek 16 bit)   */
#define PHID2    0x03   /* PHY Kimlik Kaydı   — üretici OUI (düşük 16 bit)    */
#define PHCON2   0x10   /* PHY Kontrol Kaydı 2 — HDLDIS (yarı-duplex devre dışı) */
#define PHSTAT2  0x11   /* PHY Durum Kaydı 2  — bağlantı değişim biti         */

/* ----- Chip Select (CS) makroları -----
 * ENC28J60 SPI iletişimini aktif-düşük CS piniyle seçer.
 * PB6 pini STM32 tarafında CS olarak atanmıştır. */
#define ENC28J60_CS_LOW()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)
#define ENC28J60_CS_HIGH()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)

/* ----- Fonksiyon bildirimleri ----- */

/* Tek bir SPI opcode + adres + veri baytı gönderir (yazma işlemleri için) */
void ENC28J60_WriteOp(uint8_t op, uint8_t addr, uint8_t data);

/* Tek bir SPI opcode + adres göndererek bir bayt okur */
uint8_t ENC28J60_ReadOp(uint8_t op, uint8_t addr);

/* ECON1.BSEL bitleri aracılığıyla aktif register bankasını seçer (0–3) */
void ENC28J60_SetBank(uint8_t bank);

/* RCR (Read Control Register) opcode'u ile register okur */
uint8_t ENC28J60_ReadReg(uint8_t addr);

/* WCR (Write Control Register) opcode'u ile register yazar */
void ENC28J60_WriteReg(uint8_t addr, uint8_t data);

/* MII arayüzü üzerinden 16-bit PHY register değeri okur */
uint16_t ENC28J60_ReadPHY(uint8_t phyAddr);

/* MII arayüzü üzerinden 16-bit PHY register değeri yazar */
void ENC28J60_WritePHY(uint8_t phyAddr, uint16_t data);

/* ENC28J60'ı başlatır: soft reset, tampon yapılandırması, MAC adresi yazımı, PHY ayarı */
void ENC28J60_Init(void);

/* RBM (Read Buffer Memory) komutu ile SPI üzerinden tampondan veri okur */
void ENC28J60_ReadBuffer(uint8_t *buf, uint16_t len);

/* WBM (Write Buffer Memory) komutu ile SPI üzerinden tampona veri yazar */
void ENC28J60_WriteBuffer(uint8_t *buf, uint16_t len);

/* Gelen Ethernet çerçevesini okur; paket yoksa 0 döner */
uint16_t ENC28J60_PacketReceive(uint8_t *buf, uint16_t maxlen);

/* Ethernet çerçevesini TX tamponuna yazar ve TXRTS biti ile iletimi tetikler */
void ENC28J60_SendPacket(uint8_t *buf, uint16_t len);


#endif /* INC_ENC28J60_H_ */
