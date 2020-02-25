#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

static const struct pin_config pinconf[] = {
    {STM32_PIN_PA2, STM32F1_PINMUX_FUNC_PA2_USART2_TX},
    {STM32_PIN_PA3, STM32F1_PINMUX_FUNC_PA3_USART2_RX},
    {STM32_PIN_PA11, STM32F1_PINMUX_FUNC_PA11_USB_DM},
    {STM32_PIN_PA12, STM32F1_PINMUX_FUNC_PA12_USB_DP},
};

static int pinmux_stm32_init(struct device *port) {
  ARG_UNUSED(port);

  stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

  return 0;
}

SYS_INIT(pinmux_stm32_init, POST_KERNEL,
         CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
