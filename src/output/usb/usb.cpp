#include "output/usb/usb.h"

#include <stdint.h>

#include <zephyr.h>

#include <init.h>
#include <logging/log.h>

#include <usb/class/usb_hid.h>
#include <usb/usb_device.h>

#include "arch.h"
#include "output/output.h"
#include "output/usb/hid.h"
#include "output/usb/nx/hid.h"
#include "output/usb/ps4/hid.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(usb);

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_SWITCH)
static nx::Hid nx_hid;
#endif

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
static ps4::Hid ps4_hid;
#endif

#if defined(CONFIG_USB_DC_STM32)
// STM32 doesn't support disabling USB yet, so we need to save our probe state and reboot.
enum class ProbeResult : uint64_t {
  NX = 0xAAAAAAAAAAAAAAAA,
  PS4 = 0x5555555555555555,
};

static ProbeResult probe_result __attribute__((section(".noinit")));
#endif

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4) && defined(CONFIG_PASSINGLINK_OUTPUT_USB_SWITCH)
static Hid* usb_probe() {
#if defined(CONFIG_USB_DC_STM32)
  ProbeResult previous_probe_result = probe_result;
  probe_result = static_cast<ProbeResult>(0);

  if (previous_probe_result == ProbeResult::NX) {
    LOG_WRN("previous probe detected Switch");
    return &nx_hid;
  } else if (previous_probe_result == ProbeResult::PS4) {
    LOG_WRN("previous probe detected PS4");
    return &ps4_hid;
  }
#endif

  LOG_INF("probing for USB profiles");

  // Default to PS4 if we have no clue.
  Hid* result = &ps4_hid;

  // We need to register some no-op hid ops, or we'll explode.
  struct hid_ops ops = {};

  struct device* usb_hid_device = device_get_binding("HID_0");
  span<const u8_t> report_descriptor = nx_hid.ReportDescriptor();
  usb_hid_register_device(usb_hid_device, report_descriptor.data(), report_descriptor.size(), &ops);

  // The specific signature we're looking for is the Switch preemptively clearing halts
  // on input and output endpoints.
  static int clear_halts = 0;
  usb_enable([](enum usb_dc_status_code status, const u8_t* param) {
    if (status == USB_DC_CLEAR_HALT) {
      LOG_WRN("halt");
      ++clear_halts;
    }
  });

  k_sleep(1000);

  if (clear_halts == 2) {
    LOG_INF("probe detected Switch");
    result = &nx_hid;
  } else {
    LOG_INF("probe didn't detect Switch, defaulting to PS4");
  }

#if defined(CONFIG_USB_DC_STM32)
  // usb_disable isn't implemented for STM32, so we need to stash our result and reboot.
  probe_result = (result == &nx_hid) ? ProbeResult::NX : ProbeResult::PS4;
  reboot();
#else
  // Clean up after ourselves.
  usb_disable();
  usb_hid_unregister_device(usb_hid_device);
#endif

  return result;
}
#endif

namespace passinglink {

int usb_init() {
#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4) && defined(CONFIG_PASSINGLINK_OUTPUT_USB_SWITCH)
  // TODO: Add support for holding a button to force a profile.
  Hid* hid = usb_probe();
#elif defined(CONFIG_PASSINGLINK_OUTPUT_USB_SWITCH)
  Hid* hid = &nx_hid;
#elif defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
  Hid* hid = &ps4_hid;
#endif

  if (!hid) {
    LOG_WRN("no USB HID output available");
    return -1;
  }

  return passinglink::usb_hid_init(hid);
}

}  // namespace passinglink
