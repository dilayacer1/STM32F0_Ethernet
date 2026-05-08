/*
 * enc28j60.c
 *
 *  Created on: May 8, 2026
 *      Author: DilayACER
 *
 * ENC28J60 SPI Ethernet denetleyicisi için düşük seviyeli sürücü.
 * Tüm SPI iletişimi, register okuma/yazma, PHY erişimi, başlatma,
 * paket alma ve paket gönderme işlemleri bu dosyada gerçekleştirilir.
 *
 * Donanım bağlantısı: STM32F0 — SPI1 (SCK/MISO/MOSI) + PB6 (CS)
 */

#include "enc28j60.h"

/* SPI handle ve MAC adresi main.c / enc_netif.c tarafında tanımlanmıştır */
extern SPI_HandleTypeDef hspi1;
extern uint8_t mac_addr[6];

/*
 * ENC28J60_WriteOp
 * SPI üzerinden tek bir opcode + adres + veri baytı gönderir.
 * ENC28J60, komut baytını [opcode(3 bit) | adres(5 bit)] biçiminde bekler.
 *   op   : SPI opcode (örn. 0x40 = WCR, 0x80 = BFS, 0xA0 = BFC)
 *   addr : hedef register adresi (5-bit)
 *   data : yazılacak değer
 */
void ENC28J60_WriteOp(uint8_t op, uint8_t addr, uint8_t data)
{
    uint8_t tx = op | (addr & 0x1F);  /* komut baytını oluştur */

    ENC28J60_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &tx, 1, HAL_MAX_DELAY);   /* opcode + adres gönder */
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY); /* veri baytını gönder   */
    ENC28J60_CS_HIGH();
}

/*
 * ENC28J60_ReadOp
 * SPI üzerinden tek bir opcode + adres göndererek register değeri okur.
 * ENC28J60 veriyi bir sonraki SPI clock'ta gönderir; bu bayt alınır.
 *   op   : SPI opcode (örn. 0x00 = RCR)
 *   addr : okunacak register adresi (5-bit)
 * Dönüş: okunan 8-bit register değeri
 */
uint8_t ENC28J60_ReadOp(uint8_t op, uint8_t addr)
{
    uint8_t tx = op | (addr & 0x1F);
    uint8_t rx = 0;

    ENC28J60_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &tx, 1, HAL_MAX_DELAY);

    /* ENC28J60 veriyi bir dummy byte sonrasında gönderir */
    HAL_SPI_Receive(&hspi1, &rx, 1, HAL_MAX_DELAY);

    ENC28J60_CS_HIGH();

    return rx;
}

/*
 * ENC28J60_SetBank
 * ECON1 yazacının BSEL1:BSEL0 bitleri aracılığıyla aktif register bankasını seçer.
 * ENC28J60'ın register haritası 4 bankaya bölünmüştür (0–3); her banka farklı
 * işlevsel register kümesini içerir. Banka değiştirilmeden önce mevcut banka temizlenir.
 */
void ENC28J60_SetBank(uint8_t bank)
{
    /* BFC (0xA0): ECON1'deki BSEL1:BSEL0 bitlerini temizle */
    ENC28J60_WriteOp(0xA0, 0x1F, 0x03);
    /* BFS (0x80): istenen bankayı ECON1'e yaz */
    ENC28J60_WriteOp(0x80, 0x1F, bank & 0x03);
}

/*
 * ENC28J60_ReadReg
 * RCR (Read Control Register) opcode'u (0x00) ile belirtilen register adresini okur.
 * Önce SetBank ile doğru bankaya geçilmiş olmalıdır.
 */
uint8_t ENC28J60_ReadReg(uint8_t addr)
{
    return ENC28J60_ReadOp(0x00, addr);
}

/*
 * ENC28J60_WriteReg
 * WCR (Write Control Register) opcode'u (0x40) ile belirtilen register adresine değer yazar.
 * Önce SetBank ile doğru bankaya geçilmiş olmalıdır.
 */
void ENC28J60_WriteReg(uint8_t addr, uint8_t data)
{
    ENC28J60_WriteOp(0x40, addr, data);
}

/*
 * ENC28J60_ReadPHY
 * MII (Media Independent Interface) protokolü aracılığıyla 16-bit PHY register okur.
 * PHY registerları SPI ile doğrudan erişilemez; MIREGADR, MICMD ve MIRD registerları
 * üzerinden dolaylı erişim gerektirir.
 *   phyAddr : PHY register adresi (örn. PHCON1=0x00, PHSTAT1=0x01)
 * Dönüş: 16-bit PHY register değeri (düşük bayt + yüksek bayt birleşik)
 */
uint16_t ENC28J60_ReadPHY(uint8_t phyAddr)
{
    ENC28J60_SetBank(2);

    ENC28J60_WriteReg(0x14, phyAddr);   /* MIREGADR: hedef PHY register adresini yaz */
    ENC28J60_WriteReg(0x12, 0x01);      /* MICMD.MIIRD = 1: okuma işlemini başlat    */

    /* Bank 3'e geçerek MISTAT.BUSY bitini izle */
    ENC28J60_SetBank(3);

    uint32_t timeout = HAL_GetTick();

    /* MII okuma işlemi tamamlanana kadar bekle (max 100 ms) */
    while (ENC28J60_ReadReg(0x0A) & 0x01)
    {
        if (HAL_GetTick() - timeout > 100) break;
    }

    ENC28J60_SetBank(2);
    ENC28J60_WriteReg(0x12, 0x00); /* MICMD.MIIRD = 0: okuma bayrağını temizle */

    /* MIRDL ve MIRDH registerlarından 16-bit değeri oluştur */
    uint16_t val;
    val  = ENC28J60_ReadReg(0x18);                      /* MIRDL: düşük 8 bit */
    val |= ((uint16_t)ENC28J60_ReadReg(0x19) << 8);     /* MIRDH: yüksek 8 bit */

    return val;
}

/*
 * ENC28J60_WritePHY
 * MII protokolü aracılığıyla 16-bit PHY register yazar.
 * MIWRL önce, MIWRH sonra yazılmalıdır; MIWRH yazımı iletimi tetikler.
 * İşlem tamamlanana kadar MISTAT.BUSY biti izlenir.
 *   phyAddr : PHY register adresi
 *   data    : yazılacak 16-bit değer
 */
void ENC28J60_WritePHY(uint8_t phyAddr, uint16_t data)
{
    /* Bank 2: MII register'larına erişim */
    ENC28J60_SetBank(2);

    /* MIREGADR: hedef PHY register adresini seç */
    ENC28J60_WriteReg(0x14, phyAddr);

    /* MIWRL önce yazılır; MIWRH yazımı yazma işlemini tetikler */
    ENC28J60_WriteReg(0x16, data & 0xFF);        /* MIWRL: düşük 8 bit */
    ENC28J60_WriteReg(0x17, (data >> 8) & 0xFF); /* MIWRH: yüksek 8 bit — trigger */

    /* Bank 3: MISTAT.BUSY bitini izle */
    ENC28J60_SetBank(3);

    uint32_t timeout = HAL_GetTick();

    /* MII yazma işlemi tamamlanana kadar bekle (max 100 ms) */
    while (ENC28J60_ReadReg(0x0A) & 0x01) /* MISTAT.BUSY */
    {
        if (HAL_GetTick() - timeout > 100)
            break;
    }
}

/*
 * ENC28J60_Init
 * ENC28J60 Ethernet denetleyicisini sıfırdan yapılandırır.
 * Adımlar sırasıyla:
 *   1. Soft reset gönder (0xFF komutu)
 *   2. CLKRDY biti ile osilatörün hazır olmasını bekle
 *   3. RX tamponunu (0x0000–0x0BFF) yapılandır
 *   4. MAC katmanını etkinleştir (MACON1/3/4) ve maksimum çerçeve uzunluğunu ayarla
 *   5. MAC adresini MAADR registerlarına yaz
 *   6. PHY tam-çift modunu etkinleştir (PHCON1.PDPXMD)
 *   7. Alım filtresini ayarla ve ECON1.RXEN ile alımı başlat
 */
void ENC28J60_Init(void)
{
    /* 1. Soft reset: 0xFF komutu tüm registerları varsayılana döndürür */
    ENC28J60_CS_LOW();
    uint8_t cmd = 0xFF;
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    ENC28J60_CS_HIGH();

    HAL_Delay(50); /* reset sonrası osilatör stabilizasyonu için bekle */

    /* 2. ESTAT.CLKRDY: dahili osilatör hazır olana kadar bekle (bit 0) */
    while (!(ENC28J60_ReadReg(0x1D) & 0x01));

    /* 3. RX dairesel tampon sınırlarını ayarla (Bank 0) */
    ENC28J60_SetBank(0);
    ENC28J60_WriteReg(0x08, RXSTART & 0xFF);  /* ERXSTL */
    ENC28J60_WriteReg(0x09, RXSTART >> 8);    /* ERXSTH */
    ENC28J60_WriteReg(0x0A, RXEND & 0xFF);    /* ERXNDL */
    ENC28J60_WriteReg(0x0B, RXEND >> 8);      /* ERXNDH */

    /* 4. MAC katmanı yapılandırması (Bank 2) */
    ENC28J60_SetBank(2);
    /* MACON1: MARXEN (MAC RX etkin), TXPAUS, RXPAUS, PASSALL */
    ENC28J60_WriteReg(0x00, 0x0D);

    /* MACON3: tam-duplex, otomatik dolgu, CRC oluşturma, çerçeve uzunluğu kontrolü */
    ENC28J60_WriteReg(0x02, 0xF3);

    /* MACON4: DEFER biti — gönderim sırasında ortam meşgulse ertele */
    ENC28J60_WriteReg(0x03, 0x40);

    /* MAMXFL: maksimum çerçeve uzunluğu = 0x05EE (1518 byte, standart Ethernet) */
    ENC28J60_WriteReg(0x0A, 0xEE);  /* MAMXFLL */
    ENC28J60_WriteReg(0x0B, 0x05);  /* MAMXFLH */

    /* 5. MAC adresi (Bank 3): MAADR registerlarına 6 bayt yaz */
    ENC28J60_SetBank(3);
    ENC28J60_WriteReg(0x04, mac_addr[0]); /* MAADR1 */
    ENC28J60_WriteReg(0x05, mac_addr[1]); /* MAADR2 */
    ENC28J60_WriteReg(0x00, mac_addr[2]); /* MAADR3 */
    ENC28J60_WriteReg(0x01, mac_addr[3]); /* MAADR4 */
    ENC28J60_WriteReg(0x02, mac_addr[4]); /* MAADR5 */
    ENC28J60_WriteReg(0x03, mac_addr[5]); /* MAADR6 */

    /* 6. PHY tam-çift modu: PHCON1.PDPXMD (bit 8) set */
    ENC28J60_WritePHY(0x00, 0x0100);

    /* 7. RX filtresi: unicast + broadcast + CRC kontrolü (Bank 1, ERXFCON) */
    ENC28J60_SetBank(1);
    ENC28J60_WriteReg(0x18, 0x00); /* ERXFCON: tüm filtreler kapalı — her paketi kabul et */

    /* ECON1.RXEN (bit 2) set ederek alımı etkinleştir */
    ENC28J60_SetBank(0);
    ENC28J60_WriteOp(0x80, 0x1F, 0x04); /* BFS ECON1 RXEN */
}

/*
 * ENC28J60_ReadBuffer
 * RBM (Read Buffer Memory, 0x3A) komutu ile ENC28J60 dahili SRAM'inden
 * SPI üzerinden toplu veri okur. ERDPT yazmacı hangi adresten okunacağını belirler.
 *   buf : okunan verinin yazılacağı hedef tampon
 *   len : okunacak bayt sayısı
 */
void ENC28J60_ReadBuffer(uint8_t *buf, uint16_t len)
{
    uint8_t cmd = 0x3A; /* RBM opcode */
    ENC28J60_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY); /* RBM komutu gönder */
    HAL_SPI_Receive(&hspi1, buf, len, HAL_MAX_DELAY); /* veriyi oku        */
    ENC28J60_CS_HIGH();
}

/*
 * ENC28J60_WriteBuffer
 * WBM (Write Buffer Memory, 0x7A) komutu ile ENC28J60 dahili SRAM'ine
 * SPI üzerinden toplu veri yazar. EWRPT yazmacı hangi adrese yazılacağını belirler.
 *   buf : gönderilecek verinin bulunduğu kaynak tampon
 *   len : yazılacak bayt sayısı
 */
void ENC28J60_WriteBuffer(uint8_t *buf, uint16_t len)
{
    uint8_t cmd = 0x7A; /* WBM opcode */
    ENC28J60_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);  /* WBM komutu gönder */
    HAL_SPI_Transmit(&hspi1, buf, len, HAL_MAX_DELAY); /* veriyi yaz        */
    ENC28J60_CS_HIGH();
}

/* rxPtr: dairesel RX tamponunda bir sonraki paketin başlangıç adresi */
static uint16_t rxPtr = RXSTART;

/*
 * ENC28J60_PacketReceive
 * RX tamponundan bir Ethernet çerçevesi okur ve üst katmana teslim eder.
 * ENC28J60, her paketten önce 6 baytlık donanım başlığı ekler:
 *   [nextPtr:2][durum:2][uzunluk:2]
 * FCS (4 byte) paket uzunluğundan çıkarılır.
 * Tampon yönetimi: ERXRDPT güncellenerek okunan alan serbest bırakılır,
 * ECON2.PKTDEC ile paket sayacı azaltılır.
 *   buf    : Ethernet çerçevesinin yazılacağı hedef tampon
 *   maxlen : tamponun maksimum kapasitesi (taşmayı önler)
 * Dönüş: okunan çerçevenin bayt cinsinden uzunluğu; paket yoksa 0
 */
uint16_t ENC28J60_PacketReceive(uint8_t *buf, uint16_t maxlen)
{
    /* EPKTCNT: bekleyen paket sayısını kontrol et (Bank 1) */
    ENC28J60_SetBank(1);
    uint8_t pktCnt = ENC28J60_ReadReg(0x19);
    if (pktCnt == 0) return 0;

    /* ERDPT'yi rxPtr konumuna ayarla — okuma bu adresten başlar */
    ENC28J60_SetBank(0);
    ENC28J60_WriteReg(0x00, rxPtr & 0xFF);        /* ERDPTL */
    ENC28J60_WriteReg(0x01, (rxPtr >> 8) & 0xFF); /* ERDPTH */

    /* 6 baytlık donanım başlığını oku */
    uint8_t header[6];
    ENC28J60_ReadBuffer(header, 6);

    uint16_t nextPtr = header[0] | (header[1] << 8); /* bir sonraki paketin adresi */
    uint16_t pktLen  = header[4] | (header[5] << 8); /* çerçeve uzunluğu (FCS dahil) */

    /* FCS 4 baytını çıkar — uygulama katmanına aktarılmaz */
    pktLen -= 4;

    /* Tampon taşmasını önle */
    if (pktLen > maxlen) pktLen = maxlen;
    ENC28J60_ReadBuffer(buf, pktLen); /* Ethernet çerçevesini oku */

    /* ERXRDPT güncelle: okunan alanı serbest bırak (dairesel tampon yönetimi) */
    rxPtr = nextPtr;
    ENC28J60_SetBank(0);
    ENC28J60_WriteReg(0x0C, (nextPtr - 1) & 0xFF);         /* ERXRDPTL */
    ENC28J60_WriteReg(0x0D, ((nextPtr - 1) >> 8) & 0xFF);  /* ERXRDPTH */

    /* ECON2.PKTDEC (bit 6): donanım paket sayacını bir azalt */
    ENC28J60_WriteOp(0x80, 0x1E, 0x40); /* BFS ECON2 PKTDEC */

    return pktLen;
}

/*
 * ENC28J60_SendPacket
 * Bir Ethernet çerçevesini TX tamponuna yazar ve iletimi başlatır.
 * İşlem adımları:
 *   1. Önceki TX'i iptal etmek için TXRTS bitini temizle
 *   2. ETXST ve EWRPT pointerlarını TXSTART'a ayarla
 *   3. Per-packet kontrol baytı (0x00 = varsayılan ayarlar) + çerçeveyi yaz
 *   4. ETXND'yi hesapla ve yaz
 *   5. TXRTS bitini set ederek gönderimi tetikle
 *   6. TXRTS biti düşene kadar bekle (gönderim tamamlandı)
 *   buf : gönderilecek Ethernet çerçevesi
 *   len : çerçeve uzunluğu (bayt)
 */
void ENC28J60_SendPacket(uint8_t *buf, uint16_t len)
{
    /* ECON1.TXRTS'yi temizle — önceki olası hatalı TX'i iptal et */
    ENC28J60_WriteOp(0xA0, 0x1F, 0x08); /* BFC ECON1 TXRTS */

    ENC28J60_SetBank(0);

    /* ETXST: TX tamponunun başlangıç adresini ayarla */
    ENC28J60_WriteReg(0x04, TXSTART & 0xFF); /* ETXSTL */
    ENC28J60_WriteReg(0x05, TXSTART >> 8);   /* ETXSTH */

    /* EWRPT: yazma pointer'ını TX tamponunun başına al */
    ENC28J60_WriteReg(0x02, TXSTART & 0xFF); /* EWRPTL */
    ENC28J60_WriteReg(0x03, TXSTART >> 8);   /* EWRPTH */

    /* WBM komutuyla per-packet kontrol baytı + Ethernet çerçevesini yaz */
    uint8_t cmd  = 0x7A; /* WBM opcode */
    uint8_t ctrl = 0x00; /* per-packet kontrol: varsayılan ayarlar (MACON3 geçerli) */
    ENC28J60_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd,  1,   HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, &ctrl, 1,   HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, buf,   len, HAL_MAX_DELAY);
    ENC28J60_CS_HIGH();

    /* ETXND: TX tamponunun bitiş adresini hesapla (kontrol baytı dahil) */
    uint16_t txEnd = TXSTART + len;
    ENC28J60_WriteReg(0x06, txEnd & 0xFF); /* ETXNDL */
    ENC28J60_WriteReg(0x07, txEnd >> 8);   /* ETXNDH */

    /* ECON1.TXRTS set: donanım gönderimi başlat */
    ENC28J60_WriteOp(0x80, 0x1F, 0x08); /* BFS ECON1 TXRTS */

    /* Gönderim tamamlanana kadar bekle: TXRTS biti düşer */
    uint32_t t = HAL_GetTick();
    ENC28J60_SetBank(0);
    while (ENC28J60_ReadReg(0x1F) & 0x08) /* ECON1.TXRTS hâlâ set mi? */
    {
        if (HAL_GetTick() - t > 200) break; /* 200 ms zaman aşımı */
    }
}
