/*
 * enc28j60.c
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 */

#include "enc28j60.h"

extern SPI_HandleTypeDef hspi1;
extern uint8_t mac_addr[6];

void ENC28J60_WriteOp(uint8_t op, uint8_t addr, uint8_t data)
{
    uint8_t tx = op | (addr & 0x1F);

    ENC28J60_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &tx, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
    ENC28J60_CS_HIGH();
}

uint8_t ENC28J60_ReadOp(uint8_t op, uint8_t addr)
{
    uint8_t tx = op | (addr & 0x1F);
    uint8_t rx = 0;

    ENC28J60_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &tx, 1, HAL_MAX_DELAY);

    // dummy byte (ENC28J60 şart)
    HAL_SPI_Receive(&hspi1, &rx, 1, HAL_MAX_DELAY);

    ENC28J60_CS_HIGH();

    return rx;
}
void ENC28J60_SetBank(uint8_t bank)
{
    // BFC: opcode 0xA0, ECON1 adresi 0x1F, BSEL bitleri 0x03
    ENC28J60_WriteOp(0xA0, 0x1F, 0x03); // BSEL1:BSEL0 temizle
    // BFS: opcode 0x80, ECON1 adresi 0x1F, istenen bank
    ENC28J60_WriteOp(0x80, 0x1F, bank & 0x03); // bank set
}
uint8_t ENC28J60_ReadReg(uint8_t addr)
{
    return ENC28J60_ReadOp(0x00, addr);
}

void ENC28J60_WriteReg(uint8_t addr, uint8_t data)
{
    ENC28J60_WriteOp(0x40, addr, data);
}
uint16_t ENC28J60_ReadPHY(uint8_t phyAddr)
{
    ENC28J60_SetBank(2);

    ENC28J60_WriteReg(0x14, phyAddr);   // MIREGADR
    ENC28J60_WriteReg(0x12, 0x01);      // MICMD.MIIRD

    // BUSY wait
    ENC28J60_SetBank(3);

    uint32_t timeout = HAL_GetTick();

    while (ENC28J60_ReadReg(0x0A) & 0x01)
    {
        if (HAL_GetTick() - timeout > 100) break;
    }

    ENC28J60_SetBank(2);
    ENC28J60_WriteReg(0x12, 0x00); // MIIRD clear

    uint16_t val;
    val  = ENC28J60_ReadReg(0x18);
    val |= ((uint16_t)ENC28J60_ReadReg(0x19) << 8);

    return val;
}

void ENC28J60_WritePHY(uint8_t phyAddr, uint16_t data)
{
    // 1) Bank 2 -> MII registerları
    ENC28J60_SetBank(2);

    // PHY register adresini seç (MIREGADR)
    ENC28J60_WriteReg(0x14, phyAddr);

    // 2) Data yaz (MIWRL önce)
    ENC28J60_WriteReg(0x16, data & 0xFF);        // MIWRL
    ENC28J60_WriteReg(0x17, (data >> 8) & 0xFF); // MIWRH → write trigger

    // 3) Write işlemi başlar, BUSY bekle
    ENC28J60_SetBank(3);

    uint32_t timeout = HAL_GetTick();

    while (ENC28J60_ReadReg(0x0A) & 0x01) // MISTAT.BUSY
    {
        if (HAL_GetTick() - timeout > 100)
            break;
    }
}
void ENC28J60_Init(void)
{
    // Soft reset
    ENC28J60_CS_LOW();
    uint8_t cmd = 0xFF;
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    ENC28J60_CS_HIGH();

    HAL_Delay(50);

    // wait CLKRDY
    while (!(ENC28J60_ReadReg(0x1D) & 0x01));

    // RX buffer
    ENC28J60_SetBank(0);
    ENC28J60_WriteReg(0x08, RXSTART & 0xFF);
    ENC28J60_WriteReg(0x09, RXSTART >> 8);
    ENC28J60_WriteReg(0x0A, RXEND & 0xFF);
    ENC28J60_WriteReg(0x0B, RXEND >> 8);

    // MACON1
    ENC28J60_SetBank(2);
    ENC28J60_WriteReg(0x00, 0x0D);

    // MACON3
    ENC28J60_WriteReg(0x02, 0xF3);

    // MACON4 (çok kritik)
    ENC28J60_WriteReg(0x03, 0x40);

    // MAMXFL
    ENC28J60_WriteReg(0x0A, 0xEE);
    ENC28J60_WriteReg(0x0B, 0x05);

    // MAC address
    ENC28J60_SetBank(3);
    ENC28J60_WriteReg(0x04, mac_addr[0]);
    ENC28J60_WriteReg(0x05, mac_addr[1]);
    ENC28J60_WriteReg(0x00, mac_addr[2]);
    ENC28J60_WriteReg(0x01, mac_addr[3]);
    ENC28J60_WriteReg(0x02, mac_addr[4]);
    ENC28J60_WriteReg(0x03, mac_addr[5]);

    // PHY full duplex
    ENC28J60_WritePHY(0x00, 0x0100);
    // RX filtresi: unicast + broadcast + CRC kontrolü
    ENC28J60_SetBank(1);
    ENC28J60_WriteReg(0x18, 0x00);
    // RX enable
    ENC28J60_SetBank(0);
    ENC28J60_WriteOp(0x80, 0x1F, 0x04); // ECON1 RXEN
}

void ENC28J60_ReadBuffer(uint8_t *buf, uint16_t len)
{
    uint8_t cmd = 0x3A;
    ENC28J60_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, buf, len, HAL_MAX_DELAY);
    ENC28J60_CS_HIGH();
}

void ENC28J60_WriteBuffer(uint8_t *buf, uint16_t len)
{
    uint8_t cmd = 0x7A; // WBM
    ENC28J60_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, buf, len, HAL_MAX_DELAY);
    ENC28J60_CS_HIGH();
}
// Global — bir sonraki paketin adresi
static uint16_t rxPtr = RXSTART;

uint16_t ENC28J60_PacketReceive(uint8_t *buf, uint16_t maxlen)
{
    // Paket var mı kontrol et
    ENC28J60_SetBank(1);
    uint8_t pktCnt = ENC28J60_ReadReg(0x19); // EPKTCNT
    if (pktCnt == 0) return 0;

    // ERDPT'yi rxPtr'ye ayarla
    ENC28J60_SetBank(0);
    ENC28J60_WriteReg(0x00, rxPtr & 0xFF);        // ERDPTL
    ENC28J60_WriteReg(0x01, (rxPtr >> 8) & 0xFF); // ERDPTH

    // 6 byte header oku
    uint8_t header[6];
    ENC28J60_ReadBuffer(header, 6);

    uint16_t nextPtr = header[0] | (header[1] << 8);
    uint16_t pktLen  = header[4] | (header[5] << 8);

    // FCS 4 byte'ı çıkar
    pktLen -= 4;

    // Paketi oku
    if (pktLen > maxlen) pktLen = maxlen;
    ENC28J60_ReadBuffer(buf, pktLen);

    // ERXRDPT güncelle — buffer'ı serbest bırak
    rxPtr = nextPtr;
    ENC28J60_SetBank(0);
    ENC28J60_WriteReg(0x0C, (nextPtr - 1) & 0xFF);
    ENC28J60_WriteReg(0x0D, ((nextPtr - 1) >> 8) & 0xFF);

    // ECON2.PKTDEC — paket sayacını azalt
    ENC28J60_WriteOp(0x80, 0x1E, 0x40); // BFS ECON2 PKTDEC

    return pktLen;
}
void ENC28J60_SendPacket(uint8_t *buf, uint16_t len)
{
    // Önce TXRTS temizle
    ENC28J60_WriteOp(0xA0, 0x1F, 0x08);

    ENC28J60_SetBank(0);

    // ETXST = TXSTART
    ENC28J60_WriteReg(0x04, TXSTART & 0xFF);
    ENC28J60_WriteReg(0x05, TXSTART >> 8);

    // EWRPT = TXSTART (write pointer)
    ENC28J60_WriteReg(0x02, TXSTART & 0xFF);
    ENC28J60_WriteReg(0x03, TXSTART >> 8);

    // Per-packet control byte + veri yaz
    uint8_t cmd = 0x7A;
    uint8_t ctrl = 0x00;
    ENC28J60_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, &ctrl, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, buf, len, HAL_MAX_DELAY);
    ENC28J60_CS_HIGH();

    // ETXND = TXSTART + 1(ctrl) + len - 1
    uint16_t txEnd = TXSTART + len;
    ENC28J60_WriteReg(0x06, txEnd & 0xFF);
    ENC28J60_WriteReg(0x07, txEnd >> 8);

    // TXRTS set — gönder
    ENC28J60_WriteOp(0x80, 0x1F, 0x08);

    // Gönderim bitene kadar bekle
    uint32_t t = HAL_GetTick();
    ENC28J60_SetBank(0);
    while (ENC28J60_ReadReg(0x1F) & 0x08)
    {
        if (HAL_GetTick() - t > 200) break;
    }
}


