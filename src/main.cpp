#include <zephyr.h>

#include <init.h>
#include <logging/log.h>

#include "arch.h"

#if defined(STM32F1) || defined(STM32F3)
#include <drivers/gpio.h>
#endif

#include "bt/bt.h"
#include "input/input.h"
#include "output/output.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);

const char* init_error;
int init_rc;

#if defined(STM32F1) || defined(STM32F3)
static int stm32_usb_reset(const struct device*) {
  // BluePill board has a pull-up resistor on the D+ line.
  // Pull the D+ pin down to send a RESET condition to the USB bus.
  const struct device* gpioa = device_get_binding("GPIOA");

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

  spin(k_ms_to_cyc_ceil32(20));

  init_rc = gpio_pin_configure(gpioa, 12, GPIO_INPUT);
  if (init_rc != 0) {
    init_error = "failed to configure PA12 as input";
    return 0;
  }

  spin(k_ms_to_cyc_ceil32(20));

  return 0;
}

#define USB_RESET_PRIORITY 41
SYS_INIT(stm32_usb_reset, POST_KERNEL, USB_RESET_PRIORITY);
static_assert(CONFIG_KERNEL_INIT_PRIORITY_DEFAULT < USB_RESET_PRIORITY);
static_assert(USB_RESET_PRIORITY < CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);

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

#if defined(NRF52840)
  // Enable the trace unit so we can get a cycle count.
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#endif

  input_init();
  output_init();

#if defined(CONFIG_PASSINGLINK_BT)
  bluetooth_init();
#endif

  k_thread_priority_set(k_current_get(), CONFIG_NUM_PREEMPT_PRIORITIES - 1);
}
