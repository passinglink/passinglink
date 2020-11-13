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
      :  // No outputs.
      : "r"(cycles));
}
#endif

static void reboot_impl(k_work*) {
#if defined(CONFIG_LOG)
  while (log_buffered_cnt()) {
    k_sleep(K_MSEC(5));
  }

  // We've fed all of our log messages to the backend, but it still might take
  // some time for that to be flushed out over the wire. Sleep for a bit more
  // to give that some time to happen.
  k_sleep(K_MSEC(5));
#endif

  sys_reboot(SYS_REBOOT_WARM);
}
K_WORK_DEFINE(reboot_work, reboot_impl);

void reboot() {
  // We might be called from an ISR, schedule a reboot to happen.
  k_work_submit(&reboot_work);
}
