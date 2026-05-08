/*
 * enc28j60.h
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 */

#ifndef INC_ENC28J60_H_
#define INC_ENC28J60_H_

#include "main.h"
#include "enc_netif.h"

#define RXSTART  0x0000
#define RXEND    0x0BFF
#define TXSTART  0x0C00
// Doğru PHY register adresleri
#define PHCON1   0x00
#define PHSTAT1  0x01
#define PHID1    0x02
#define PHID2    0x03
#define PHCON2   0x10
#define PHSTAT2  0x11

#define ENC28J60_CS_LOW()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)
#define ENC28J60_CS_HIGH()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)

void ENC28J60_WriteOp(uint8_t op, uint8_t addr, uint8_t data);
uint8_t ENC28J60_ReadOp(uint8_t op, uint8_t addr);
void ENC28J60_SetBank(uint8_t bank);
uint8_t ENC28J60_ReadReg(uint8_t addr);
void ENC28J60_WriteReg(uint8_t addr, uint8_t data);
uint16_t ENC28J60_ReadPHY(uint8_t phyAddr);
void ENC28J60_WritePHY(uint8_t phyAddr, uint16_t data);
void ENC28J60_Init(void);
void ENC28J60_ReadBuffer(uint8_t *buf, uint16_t len);
void ENC28J60_WriteBuffer(uint8_t *buf, uint16_t len);
uint16_t ENC28J60_PacketReceive(uint8_t *buf, uint16_t maxlen);
void ENC28J60_SendPacket(uint8_t *buf, uint16_t len);



#endif /* INC_ENC28J60_H_ */
