#include <zephyr.h>

#include <init.h>
#include <logging/log.h>

#include "arch.h"

#if defined(STM32F1)
#include <drivers/gpio.h>
#include <dt-bindings/pinctrl/stm32-pinctrlf1.h>
#endif

#include "input/input.h"
#include "output/output.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);

const char* init_error;
int init_rc;

#if defined(STM32F1)
static int stm32_usb_reset(struct device*) {
  // BluePill board has a pull-up resistor on the D+ line.
  // Pull the D+ pin down to send a RESET condition to the USB bus.
  struct device* gpioa = device_get_binding("GPIOA");

  if (!gpioa) {
    init_error = "failed to get device binding for GPIOA";
    return 0;
  }
  init_rc = gpio_pin_configure(gpioa, 12, GPIO_OUTPUT);
  if (init_rc != 0) {
    init_error = "failed to configure PA12 as output";
    return 0;
  }

  init_rc = gpio_pin_set(gpioa, 12, 0);
  if (init_rc != 0) {
    init_error = "failed to write to GPIO pin";
    return 0;
  }

  for (int i = 0; i < 36'000; ++i) {
    asm volatile("nop");
  }

  init_rc = gpio_pin_configure(gpioa, 12, GPIO_INPUT | STM32_CNF_IN_ANALOG);
  if (init_rc != 0) {
    init_error = "failed to configure PA12 as input";
    return 0;
  }

  return 0;
}

#define USB_RESET_PRIORITY 3
SYS_INIT(stm32_usb_reset, PRE_KERNEL_1, USB_RESET_PRIORITY);
static_assert(USB_RESET_PRIORITY > CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);

#endif

// zephyrproject-rtos/zephyr#21094:
// main is renamed to zephyr_app_main via macro and must not be mangled,
// or it'll be silently ignored.
extern "C" void main(void) {
  auto kver = sys_kernel_version_get();
  LOG_INF("passinglink v0.5.0 (kernel version %d.%d.%d) initializing", SYS_KERNEL_VER_MAJOR(kver),
          SYS_KERNEL_VER_MINOR(kver), SYS_KERNEL_VER_PATCHLEVEL(kver));

  if (init_error) {
    LOG_ERR("%s: rc = %d", init_error, init_rc);
  }

  input_init();
  output_init();

  k_thread_priority_set(k_current_get(), CONFIG_NUM_PREEMPT_PRIORITIES - 1);
}
