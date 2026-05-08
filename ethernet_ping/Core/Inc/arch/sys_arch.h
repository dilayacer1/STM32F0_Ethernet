#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include <stdint.h>

typedef uint32_t sys_prot_t;

// NO_SYS=1 olduğu için bunlar typedef edilmemeli
// LwIP kendi tanımlıyor
#define SYS_MBOX_NULL   0
#define SYS_SEM_NULL    0

#endif
