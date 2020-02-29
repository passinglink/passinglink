#include "arch.h"

#include <kernel.h>

#include <logging/log_ctrl.h>
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
  while (log_buffered_cnt()) {
    k_sleep(K_MSEC(5));
  }

  sys_reboot(SYS_REBOOT_WARM);
}
