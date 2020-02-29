#pragma once

#define PL_USB_OUTPUT_COUNT                                                  \
  CONFIG_PASSINGLINK_OUTPUT_USB_SWITCH + CONFIG_PASSINGLINK_OUTPUT_USB_PS3 + \
      CONFIG_PASSINGLINK_OUTPUT_USB_PS4

namespace passinglink {

int usb_init();
void usb_reset_probe();

}  // namespace passinglink
