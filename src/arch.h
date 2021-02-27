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

#if defined(STM32F407xx)
#define STM32 1
// Seems that cube already defines STM32F4. Omitted here.
#endif

#if defined(NRF52840_XXAA)
#define NRF52840 1
#endif

#if defined(NRF52840)
static uint32_t get_cycle_count() {
  return DWT->CYCCNT;
}

static uint32_t get_cpu_freq() {
  return 64'000'000;
}
#else
static uint32_t get_cycle_count() {
  return k_cycle_get_32();
}

static uint32_t get_cpu_freq() {
  return sys_clock_hw_cycles_per_sec();
}
#endif

#if defined(__arm__)
void spin(uint32_t cycles);
#endif

void reboot();
