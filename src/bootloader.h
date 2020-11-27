#pragma once

#include <devicetree.h>

// Rebooting into DFU mode requires a separate sram device to communicate with mcuboot.
// Grep for 'mcuboot,sram_warmboot' for dts file examples.
#if DT_HAS_CHOSEN(mcuboot_sram_warmboot)

void mcuboot_enter();

#endif
