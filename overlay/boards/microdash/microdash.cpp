#include <zephyr.h>

#include <device.h>
#include <devicetree/gpio.h>
#include <drivers/gpio.h>

int microdash_init(const device*) {
#define LED_POWER_NODE DT_PATH(gpio_keys, led_power)
  const struct device* gpio_device = device_get_binding(DT_GPIO_LABEL(LED_POWER_NODE, gpios));
  if (gpio_device) {
    gpio_pin_configure(gpio_device, DT_GPIO_PIN(LED_POWER_NODE, gpios),
                       DT_GPIO_FLAGS(LED_POWER_NODE, gpios) | GPIO_OUTPUT);
    gpio_pin_set(gpio_device, DT_GPIO_PIN(LED_POWER_NODE, gpios), 1);
  }

  return 0;
}

SYS_INIT(microdash_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
