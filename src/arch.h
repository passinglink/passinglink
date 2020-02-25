#pragma once

#include <sys/types.h>

#if defined(STM32F103xB) || defined(STM32F103xE)
#define STM32 1
#define STM32F1 1
#endif

#if defined(STM32F303xC)
#define STM32 1
#define STM32F3 1
#endif

#if defined(NRF52840_XXAA)
#define NRF52840 1
#endif

void spin(uint32_t cycles);
void reboot();
