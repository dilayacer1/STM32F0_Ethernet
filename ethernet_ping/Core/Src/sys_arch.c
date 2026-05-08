#include "lwip/sys.h"
#include "stm32f0xx_hal.h"

// Zamanlayıcı — LwIP timeout için
u32_t sys_now(void)
{
    return HAL_GetTick();
}

// Interrupt koruması — NO_SYS=1 için
sys_prot_t sys_arch_protect(void)
{
    __disable_irq();
    return 0;
}

void sys_arch_unprotect(sys_prot_t pval)
{
    __enable_irq();
}
