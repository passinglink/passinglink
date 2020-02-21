#include "output/output.h"
#include "output/hid.h"
#include "output/ps4/hid.h"

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
ps4::Hid ps4_hid;
#endif

int output_init() {
#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
  return usb_hid_init(&ps4_hid);
#endif
}
