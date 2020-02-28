#include "arch.h"

#include <kernel.h>

#include <power/reboot.h>

#if defined(__arm__)
void spin(uint32_t cycles) {
  asm volatile(
    "loop:\n"
    "subs %0, #7\n"
    "bpl loop\n"
    : // No outputs.
    : "r"(cycles)
  );
}
#endif

void reboot() {
  sys_reboot(SYS_REBOOT_WARM);
}
