#include "output/output.h"

#include "output/led.h"
#include "output/usb/usb.h"

int output_init() {
  led_init();
  return passinglink::usb_init();
}
