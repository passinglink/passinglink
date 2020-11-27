#include "bootloader.h"

#include <zephyr.h>

#include <devicetree.h>
#include <power/reboot.h>
#include <shell/shell.h>

#if DT_HAS_CHOSEN(mcuboot_sram_warmboot)
bool mcuboot_available() {
  return true;
}

void mcuboot_enter() {
  static_assert(DT_REG_SIZE(DT_CHOSEN(mcuboot_sram_warmboot)) == 8);
  uint64_t* addr = reinterpret_cast<uint64_t*>(DT_REG_ADDR(DT_CHOSEN(mcuboot_sram_warmboot)));
  *addr = 0x1209214c1209214c;
  sys_reboot(SYS_REBOOT_WARM);
}

static int cmd_dfu_enter(const struct shell*, size_t, char**) {
  mcuboot_enter();
  return 0;
}

SHELL_CMD_ARG_REGISTER(dfu, NULL, "Reboot to dfu mode", cmd_dfu_enter, 0, 0);
#endif
