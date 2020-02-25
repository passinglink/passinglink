#include "arch.h"

#include <kernel.h>

#include <power/reboot.h>

void spin(uint32_t cycles) {
  asm volatile(
    "loop:\n"
    "subs %0, #7\n"
    "bpl loop\n"
    : // No outputs.
    : "r"(cycles)
  );
}

void reboot() {
  sys_reboot(SYS_REBOOT_WARM);
}
