#pragma once

#include <kernel.h>
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

#if defined(NRF52840)
static u32_t get_cycle_count() {
  return DWT->CYCCNT;
}
#else
static u32_t get_cycle_count() {
  return k_cycle_get_32();
}
#endif

#if defined(__arm__)
void spin(uint32_t cycles);
#endif

void reboot();
