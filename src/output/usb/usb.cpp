#include "output/usb/usb.h"

#include <stdint.h>

#include <zephyr.h>

#include <init.h>
#include <logging/log.h>

#include <usb/class/usb_hid.h>
#include <usb/usb_device.h>

#include "arch.h"
#include "display/display.h"
#include "input/input.h"
#include "output/led.h"
#include "output/output.h"
#include "output/usb/hid.h"
#include "output/usb/nx/hid.h"
#include "output/usb/probe_type.h"
#include "output/usb/ps3/hid.h"
#include "output/usb/ps4/hid.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(usb);

#if defined(CONFIG_PASSINGLINK_DISPLAY)
#define DISPLAY_PROBE(probing, type) display_set_connection_type(probing, type)
#else
#define DISPLAY_PROBE(probing, type) (void)0
#endif

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_SWITCH)
static NXHid nx_hid;
#endif

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS3)
static PS3Hid ps3_hid;
#endif

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
static PS4Hid ps4_hid;
#endif

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_FORCE_PROBE_REBOOT) || defined(CONFIG_USB_DC_STM32)
#define REBOOT_PROBE
#endif

static uint32_t probe_led_counter;

static optional<ProbeType> ProbeTypeFirst() {
#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_SWITCH_PROBE)
  return ProbeType::NX;
#elif defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS3_PROBE)
  return ProbeType::PS3;
#elif defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
  return ProbeType::PS4;
#else
  return {};
#endif
}

optional<ProbeType> ProbeTypeNext(ProbeType probe_type) {
  switch (probe_type) {
    case ProbeType::NX:
#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS3_PROBE)
      return ProbeType::PS3;
#elif defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
      return ProbeType::PS4;
#else
      return {};
#endif

    case ProbeType::PS3:
#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
      return ProbeType::PS4;
#else
      return {};
#endif

    default:
      return {};
  }
}

optional<Led> ProbeTypeLed(ProbeType probe_type) {
  switch (probe_type) {
    case ProbeType::NX:
      return Led::P1;
    case ProbeType::PS3:
      return Led::P2;
    case ProbeType::PS4:
      return Led::P3;
    default:
      return {};
  }
}

bool ProbeTypeIsValid(ProbeType probe_type) {
  return probe_type == ProbeType::NX || probe_type == ProbeType::PS3 ||
         probe_type == ProbeType::PS4;
}

Hid* ProbeTypeHid(ProbeType probe_type) {
#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_SWITCH)
  if (probe_type == ProbeType::NX) {
    return &nx_hid;
  }
#endif

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS3)
  if (probe_type == ProbeType::PS3) {
    return &ps3_hid;
  }
#endif

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
  if (probe_type == ProbeType::PS4) {
    return &ps4_hid;
  }
#endif

  return nullptr;
}

#if defined(REBOOT_PROBE)
static ProbeType boot_probe_type __attribute__((section(".noinit")));

optional<ProbeType> get_boot_probe() {
  if (ProbeTypeIsValid(boot_probe_type)) {
    return boot_probe_type;
  }
  return {};
}

void set_boot_probe(optional<ProbeType> probe) {
  if (probe) {
    boot_probe_type = *probe;
  } else {
    memset(&boot_probe_type, 0, sizeof(boot_probe_type));
  }
}

#endif

#if PL_USB_OUTPUT_COUNT > 1
static optional<ProbeType> current_probe;

static k_delayed_work probe_check_work;
static void usb_probe_start();
static void usb_probe_check(k_work*);

static int usb_probe() {
  k_delayed_work_init(&probe_check_work, usb_probe_check);

  optional<ProbeType> probe;
#if defined(REBOOT_PROBE)
  probe = get_boot_probe();
  if (!probe) {
    LOG_INF("no boot probe found, starting usb probe");
  } else {
    LOG_INF("boot probe found: %s", ProbeTypeHid(*probe)->Name());
  }
#endif

  // Check to see if the user is overriding detection after we check if we already probed, so
  // inadvertent button presses don't override probing.
  if (!probe) {
    RawInputState input;
    if (!input_get_raw_state(&input)) {
      LOG_ERR("failed to get input state");
    } else {
      // Only probe if the mode switch is set to PS3.
#if PL_GPIO_AVAILABLE(mode_ps3) && defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
      if (!input.mode_ps3) {
        LOG_WRN("PS4 mode switch set, selecting PS4");
        DISPLAY_PROBE(false, ProbeType::PS4);
        return passinglink::usb_hid_init(&ps4_hid);
      }
#endif

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_SWITCH)
      if (input.button_west) {
        LOG_WRN("Switch mode selected via button");
        DISPLAY_PROBE(false, ProbeType::NX);
        return passinglink::usb_hid_init(&nx_hid);
      }
#endif

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS3)
      if (input.button_north) {
        LOG_WRN("PS3 mode selected via button");
        DISPLAY_PROBE(false, ProbeType::PS3);
        return passinglink::usb_hid_init(&ps3_hid);
      }
#endif

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
      if (input.button_r1) {
        LOG_WRN("PS4 mode selected via button");
        DISPLAY_PROBE(false, ProbeType::PS4);
        return passinglink::usb_hid_init(&ps4_hid);
      }
#endif
    }
  }

  current_probe = probe;
  usb_probe_start();
  return 0;
}

static void usb_probe_start() {
  if (!current_probe) {
    current_probe = ProbeTypeFirst();
    if (!current_probe) {
      LOG_ERR("no available USB HID implementations");
      return;
    }
    DISPLAY_PROBE(true, *current_probe);
  }

  led_off(Led::P1);
  led_off(Led::P2);
  led_off(Led::P3);
  led_off(Led::P4);

  probe_led_counter = led_on(*ProbeTypeLed(*current_probe));
  Hid* current_hid = ProbeTypeHid(*current_probe);
  passinglink::usb_hid_init(current_hid);

  k_delayed_work_submit(&probe_check_work, current_hid->ProbeDelay());
}

static void usb_probe_selected() {
  auto probe = ProbeTypeLed(*current_probe);
  if (probe) {
    led_off(*probe, probe_led_counter);
  }
  led_flash(Led::P1, 1000, 300);
  led_flash(Led::P2, 1000, 300);
  led_flash(Led::P3, 1000, 300);
  led_flash(Led::P4, 1000, 300);

  DISPLAY_PROBE(false, *current_probe);
#if defined(REBOOT_PROBE)
  set_boot_probe({});
#endif
}

static void usb_probe_check(k_work*) {
  Hid* current_hid = ProbeTypeHid(*current_probe);
  LOG_INF("checking whether %s was successful", current_hid->Name());
  if (current_hid->ProbeResult()) {
    LOG_INF("%s Hid reports success", current_hid->Name());
    usb_probe_selected();
    return;
  }

  optional<ProbeType> next_probe = ProbeTypeNext(*current_probe);
  if (!next_probe) {
    LOG_ERR("%s Hid reports failure, but no more Hids are available", current_hid->Name());
    usb_probe_selected();
    return;
  }

  LOG_ERR("%s Hid reports failure, continuing", current_hid->Name());

#if defined(REBOOT_PROBE)
  // usb_disable isn't implemented for STM32, so we need to stash our result and reboot.
  set_boot_probe(next_probe);
  reboot();
#else
  passinglink::usb_hid_uninit();
  current_probe = next_probe;
  usb_probe_start();
#endif
}
#endif  // PL_USB_OUTPUT_COUNT > 1

namespace passinglink {

int usb_init() {
#if PL_USB_OUTPUT_COUNT > 1
  return usb_probe();
#elif defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS3)
  return passinglink::usb_hid_init(&ps3_hid);
#elif defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4)
  return passinglink::usb_hid_init(&ps4_hid);
#elif defined(CONFIG_PASSINGLINK_OUTPUT_USB_SWITCH)
  return passinglink::usb_hid_init(&nx_hid);
#endif

  LOG_ERR("no USB HID output available");
  return -1;
}

void usb_reset_probe() {
#if defined(CONFIG_USB_DC_STM32)
  set_boot_probe({});
#endif
}

}  // namespace passinglink
