#ifndef LWIPOPTS_H
#define LWIPOPTS_H

// OS yok
#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

// RAM kısıtlı — F051 için minimal
#define MEM_ALIGNMENT               4
//#define MEM_SIZE                    2048
//#define MEMP_NUM_PBUF               4
//#define MEMP_NUM_TCP_PCB            1
//#define MEMP_NUM_TCP_PCB_LISTEN     1
//#define MEMP_NUM_TCP_SEG            4
//#define PBUF_POOL_SIZE              4
//#define PBUF_POOL_BUFSIZE           256
#define MEM_SIZE                    2048// 1024  // 2048'den küçülttük
#define MEMP_NUM_PBUF               4     // 4'ten küçülttük
#define PBUF_POOL_SIZE              4     // 4'ten küçülttük
#define PBUF_POOL_BUFSIZE           256
#define MEMP_NUM_TCP_PCB            2
#define MEMP_NUM_TCP_PCB_LISTEN     2

#define MEMP_NUM_TCP_SEG            2     // 4'ten küçülttük
// TCP
#define LWIP_TCP                    1
#define TCP_MSS                     536
#define TCP_LISTEN_BACKLOG          1
#define TCP_SND_QUEUELEN        4
#define TCP_SND_BUF             (2 * TCP_MSS)  // 1072
#define TCP_WND                 (2 * TCP_MSS)  // 1072
// ARP
#define LWIP_ARP                    1
#define ARP_TABLE_SIZE              2
#define ARP_QUEUEING                0

// ICMP
#define LWIP_ICMP                   1

// DHCP kapalı
#define LWIP_DHCP                   0
#define LWIP_DISABLE_TCP_SANITY_CHECKS  1
// Checksum — yazılımda hesapla
#define CHECKSUM_BY_HARDWARE        0
#define LWIP_PROVIDE_ERRNO          1
#define TCP_OVERSIZE    0
#endif
