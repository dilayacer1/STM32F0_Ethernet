/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
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

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
uint8_t enc_revision = 0;
static uint8_t mac_addr[6] = {0x02, 0x00, 0x00, 0x11, 0x22, 0x33};
static uint8_t ip_addr[4] = {192, 168, 1, 100};
uint16_t button_count;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//uint8_t ENC28J60_Read_Register(uint8_t address)
//{
//    uint8_t tx[2];
//    uint8_t rx[2];
//
//    tx[0] = 0x00 | (address & 0x1F);
//    tx[1] = 0x00; // dummy
//
//    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // CS low
//    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, HAL_MAX_DELAY);
//    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   // CS high
//
//    return rx[1];
//}

static void ENC28J60_WriteOp(uint8_t op, uint8_t addr, uint8_t data)
{
    uint8_t tx = op | (addr & 0x1F);

    ENC28J60_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &tx, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
    ENC28J60_CS_HIGH();
}

static uint8_t ENC28J60_ReadOp(uint8_t op, uint8_t addr)
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

void ENC28J60_HandleARP(uint8_t *pkt)
{
    // ARP isteği mi? Opcode = 0x0001
    if (pkt[20] != 0x00 || pkt[21] != 0x01) return;

    // Hedef IP bizim IP'miz mi?
    if (pkt[38] != ip_addr[0] || pkt[39] != ip_addr[1] ||
        pkt[40] != ip_addr[2] || pkt[41] != ip_addr[3]) return;

    uint8_t reply[42];

    // Ethernet header
    memcpy(&reply[0], &pkt[6], 6);   // DST = gelen paketin kaynağı
    memcpy(&reply[6], mac_addr, 6);  // SRC = bizim MAC
    reply[12] = 0x08; reply[13] = 0x06; // EtherType ARP

    // ARP payload
    reply[14] = 0x00; reply[15] = 0x01; // HW type Ethernet
    reply[16] = 0x08; reply[17] = 0x00; // Protocol IPv4
    reply[18] = 0x06; // HW addr len
    reply[19] = 0x04; // Proto addr len
    reply[20] = 0x00; reply[21] = 0x02; // Opcode: reply

    memcpy(&reply[22], mac_addr, 6);    // Sender MAC = bizim MAC
    memcpy(&reply[28], ip_addr, 4);     // Sender IP = bizim IP

    memcpy(&reply[32], &pkt[22], 6);    // Target MAC = isteği gönderenin MAC
    memcpy(&reply[38], &pkt[28], 4);    // Target IP = isteği gönderenin IP

    ENC28J60_SendPacket(reply, 42);
}

uint16_t ip_checksum(uint8_t *buf, uint16_t len)
{
    uint32_t sum = 0;
    for (uint16_t i = 0; i < len; i += 2)
        sum += (buf[i] << 8) | buf[i+1];
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

void ENC28J60_HandleICMP(uint8_t *pkt, uint16_t len)
{
    // ICMP Echo Request mi? type=8
    if (pkt[34] != 0x08) return;

    uint16_t ip_len = (pkt[16] << 8) | pkt[17];
    uint8_t reply[600];

    // Ethernet header
    memcpy(&reply[0], &pkt[6], 6);   // DST
    memcpy(&reply[6], mac_addr, 6);  // SRC
    reply[12] = 0x08; reply[13] = 0x00; // IPv4

    // IP header
    reply[14] = 0x45; // version + IHL
    reply[15] = 0x00; // DSCP
    reply[16] = ip_len >> 8;
    reply[17] = ip_len & 0xFF;
    reply[18] = pkt[18]; reply[19] = pkt[19]; // ID
    reply[20] = 0x00; reply[21] = 0x00; // flags
    reply[22] = 0x40; // TTL=64
    reply[23] = 0x01; // protocol ICMP
    reply[24] = 0x00; reply[25] = 0x00; // checksum (sonra)
    memcpy(&reply[26], &pkt[30], 4); // src IP = pkt dst
    memcpy(&reply[30], &pkt[26], 4); // dst IP = pkt src

    // IP checksum
    uint16_t cksum = ip_checksum(&reply[14], 20);
    reply[24] = cksum >> 8;
    reply[25] = cksum & 0xFF;

    // ICMP
    reply[34] = 0x00; // type: Echo Reply
    reply[35] = 0x00; // code
    reply[36] = 0x00; reply[37] = 0x00; // checksum (sonra)
    memcpy(&reply[38], &pkt[38], ip_len - 28); // ID + seq + data

    // ICMP checksum
    uint16_t icmp_len = ip_len - 20;
    cksum = ip_checksum(&reply[34], icmp_len);
    reply[36] = cksum >> 8;
    reply[37] = cksum & 0xFF;

    ENC28J60_SendPacket(reply, 14 + ip_len);
}

uint16_t tcp_checksum(uint8_t *src_ip, uint8_t *dst_ip,
                      uint8_t *tcp_seg, uint16_t tcp_len)
{
    uint32_t sum = 0;

    // Pseudo header
    sum += (src_ip[0] << 8) | src_ip[1];
    sum += (src_ip[2] << 8) | src_ip[3];
    sum += (dst_ip[0] << 8) | dst_ip[1];
    sum += (dst_ip[2] << 8) | dst_ip[3];
    sum += 0x0006; // protocol TCP
    sum += tcp_len;

    // TCP segment
    for (uint16_t i = 0; i < tcp_len; i += 2)
    {
        if (i + 1 < tcp_len)
            sum += (tcp_seg[i] << 8) | tcp_seg[i+1];
        else
            sum += tcp_seg[i] << 8;
    }

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return ~sum;
}

void TCP_Send(uint8_t *dst_mac, uint8_t *dst_ip,
              uint16_t src_port, uint16_t dst_port,
              uint32_t seq, uint32_t ack,
              uint8_t flags,
              uint8_t *data, uint16_t data_len)
{
    uint16_t tcp_len = 20 + data_len;
    uint16_t ip_len  = 20 + tcp_len;
    uint8_t pkt[600];

    // Ethernet
    memcpy(&pkt[0], dst_mac, 6);
    memcpy(&pkt[6], mac_addr, 6);
    pkt[12] = 0x08; pkt[13] = 0x00;

    // IP
    pkt[14] = 0x45;
    pkt[15] = 0x00;
    pkt[16] = ip_len >> 8;
    pkt[17] = ip_len & 0xFF;
    pkt[18] = 0x00; pkt[19] = 0x01; // ID
    pkt[20] = 0x40; pkt[21] = 0x00; // flags: don't fragment
    pkt[22] = 0x40;                  // TTL=64
    pkt[23] = 0x06;                  // protocol TCP
    pkt[24] = 0x00; pkt[25] = 0x00; // checksum (sonra)
    memcpy(&pkt[26], ip_addr, 4);
    memcpy(&pkt[30], dst_ip, 4);

    // IP checksum
    uint16_t ck = ip_checksum(&pkt[14], 20);
    pkt[24] = ck >> 8;
    pkt[25] = ck & 0xFF;

    // TCP
    pkt[34] = src_port >> 8;   pkt[35] = src_port & 0xFF;
    pkt[36] = dst_port >> 8;   pkt[37] = dst_port & 0xFF;
    pkt[38] = seq >> 24; pkt[39] = seq >> 16;
    pkt[40] = seq >> 8;  pkt[41] = seq & 0xFF;
    pkt[42] = ack >> 24; pkt[43] = ack >> 16;
    pkt[44] = ack >> 8;  pkt[45] = ack & 0xFF;
    pkt[46] = 0x50;  // data offset = 5 (20 byte header)
    pkt[47] = flags;
    pkt[48] = 0x05; pkt[49] = 0xB4; // window = 1460
    pkt[50] = 0x00; pkt[51] = 0x00; // checksum (sonra)
    pkt[52] = 0x00; pkt[53] = 0x00; // urgent

    // Data
    if (data && data_len > 0)
        memcpy(&pkt[54], data, data_len);

    // TCP checksum
    ck = tcp_checksum(ip_addr, dst_ip, &pkt[34], tcp_len);
    pkt[50] = ck >> 8;
    pkt[51] = ck & 0xFF;

    ENC28J60_SendPacket(pkt, 14 + ip_len);
}

void ENC28J60_HandleTCP(uint8_t *pkt, uint16_t len)
{
    // Sadece port 80
    uint16_t dst_port = (pkt[36] << 8) | pkt[37];
    if (dst_port != 80) return;

    uint8_t  flags   = pkt[47];
    uint32_t seq_in  = ((uint32_t)pkt[38] << 24) | ((uint32_t)pkt[39] << 16)
                     | ((uint32_t)pkt[40] << 8)  | pkt[41];
    uint32_t ack_in  = ((uint32_t)pkt[42] << 24) | ((uint32_t)pkt[43] << 16)
                     | ((uint32_t)pkt[44] << 8)  | pkt[45];

    uint8_t *src_mac = &pkt[6];
    uint8_t *src_ip  = &pkt[26];
    uint16_t src_port = (pkt[34] << 8) | pkt[35];

    uint8_t tcp_offset = (pkt[46] >> 4) * 4;
    uint16_t payload_len = len - 14 - 20 - tcp_offset;

    // SYN geldi → SYN+ACK gönder
    if ((flags & 0x02) && !(flags & 0x10))
    {
        TCP_Send(src_mac, src_ip, 80, src_port,
                 0x12345678,      // bizim seq
                 seq_in + 1,      // ack = karşının seq + 1
                 0x12,            // SYN + ACK
                 NULL, 0);
        return;
    }

    // ACK veya PSH+ACK → GET isteği mi?
    if ((flags & 0x10) && payload_len > 0)
    {
        uint8_t *payload = &pkt[14 + 20 + tcp_offset];

        // GET isteği mi?
        if (payload[0] == 'G' && payload[1] == 'E' && payload[2] == 'T')
        {
            // Buton sayacını oku
            char html[256];
            snprintf(html, sizeof(html),
                "HTTP/1.0 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                "<html><body>"
                "<h1>Buton Sayaci: %lu</h1>"
                "<meta http-equiv='refresh' content='1'>"
                "</body></html>",
                button_count);

            uint32_t my_seq = 0x12345679;
            uint32_t my_ack = seq_in + payload_len;

            // PSH+ACK ile veri gönder
            TCP_Send(src_mac, src_ip, 80, src_port,
                     my_seq, my_ack, 0x18,
                     (uint8_t*)html, strlen(html));

            // FIN+ACK
            TCP_Send(src_mac, src_ip, 80, src_port,
                     my_seq + strlen(html), my_ack, 0x11,
                     NULL, 0);
        }
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  ENC28J60_Init();
  uint8_t test_pkt[42] = {
      // DST: broadcast
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
      // SRC: bizim MAC
      0x02,0x00,0x00,0x11,0x22,0x33,
      // EtherType: ARP
      0x08,0x06,
      // ARP
      0x00,0x01, // HW type
      0x08,0x00, // Protocol
      0x06,0x04, // lengths
      0x00,0x01, // opcode request
      0x02,0x00,0x00,0x11,0x22,0x33, // sender MAC
      192,168,1,100,                  // sender IP
      0x00,0x00,0x00,0x00,0x00,0x00, // target MAC
      192,168,1,50                    // target IP
  };

  uint8_t pkt_buf[600];
  uint8_t tx = 0xAA;
  uint8_t rx = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


	  uint16_t len = ENC28J60_PacketReceive(pkt_buf, sizeof(pkt_buf));
	  if (len > 0)
	  {
	      uint16_t ethertype = (pkt_buf[12] << 8) | pkt_buf[13];
	      if (ethertype == 0x0806)
	      {
	          ENC28J60_HandleARP(pkt_buf);
	      }
	      else if (ethertype == 0x0800)
	      {
	          if (pkt_buf[23] == 0x01) // ICMP
	          {
	              ENC28J60_HandleICMP(pkt_buf, len);
	          }else if(pkt_buf[23] == 0x06){
	        	  ENC28J60_HandleTCP(pkt_buf, len);
	          }
	      }
	  }

	  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
	  {
	      button_count++;
	      HAL_Delay(200); // debounce
	  }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LD4_Pin|LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_SET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PB6 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
