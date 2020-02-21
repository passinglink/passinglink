#include "output/output.h"

#include "output/usb/usb.h"

int output_init() {
  return passinglink::usb_init();
}
