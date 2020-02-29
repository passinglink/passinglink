#include <zephyr.h>

#include <init.h>
#include <logging/log.h>

#include <usb/class/usb_hid.h>
#include <usb/usb_device.h>

#include "output/output.h"
#include "output/usb/hid.h"
#include "output/usb/nx/hid.h"
#include "output/usb/ps4/hid.h"
#include "output/usb/usb.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(hid);

#include "arch.h"
#include "profiling.h"

static Hid* hid;
static struct device* usb_hid_device;

static optional<s64_t> suspend_timestamp;
static struct k_delayed_work delayed_write_work;

// USB transfers works on a host-polled basis: we put data into registers for
// the hardware to send to the host. When this data gets succesfully sent, we
// get notified to fill the bin with more data. As a result, if we immediately
// write when able, we increase input latency by up to the polling interval.
//
// Attempt to reduce this latency by scheduling a write to happen after a fixed
// interval, which reduces average latency by that amount, as long as it's short
// enough that we actually manage to finish the write before the interval elapses.
//
// TODO: Instead of using work queue timing (with ~100us precision), use a timer
//       interrupt which is far more precise?
#if defined(STM32)
constexpr u32_t HID_REPORT_INTERVAL_US = 700;
#elif defined(NRF52840)
constexpr u32_t HID_REPORT_INTERVAL_US = 600;
#else
#warning HID_REPORT_INTERVAL_US unset, defaulting to 0
constexpr u32_t HID_REPORT_INTERVAL_US = 0;
#endif

constexpr u32_t HID_REPORT_INTERVAL_TICKS = k_us_to_ticks_ceil32(HID_REPORT_INTERVAL_US);

static void submit_write() {
  k_delayed_work_submit_ticks(&delayed_write_work, HID_REPORT_INTERVAL_TICKS);
}

static void write_report(struct k_work* item = nullptr) {
  u8_t report_buf[64];

  // TODO: Optimize GetReport: it's taking >100us to create the report to send.
  ssize_t report_size;
  {
    PROFILE("Hid::GetReport", 128);
    report_size = hid->GetReport(HidReportType::Input, 1, span(report_buf, sizeof(report_buf)));
    if (report_size < 0) {
      return;
    }
  }

  size_t bytes_written = 0;
  int rc = hid_int_ep_write(usb_hid_device, report_buf, report_size, &bytes_written);
  if (rc < 0) {
    LOG_ERR("USB write failed, requeuing: rc = %d", rc);
    submit_write();
  } else if (bytes_written != static_cast<size_t>(report_size)) {
    LOG_WRN("wrote fewer bytes (%d) than expected (%d): buffer full?", bytes_written, report_size);
  }

#if defined(INTERVAL_PROFILING)
  static u32_t previous;
  u32_t now = k_cycle_get_32();
  u32_t diff = now - previous;
  previous = now;

  if (diff > 100'000 && diff < 72'000'000) {
    LOG_ERR("interval between writes too long: %d cycles", diff);
  }
#endif
}

static void usb_status_cb(enum usb_dc_status_code status, const u8_t* param) {
  switch (status) {
    case USB_DC_ERROR:
      LOG_INF("USB_DC_ERROR");
      break;
    case USB_DC_RESET:
      LOG_INF("USB_DC_RESET");
      break;
    case USB_DC_CONNECTED:
      LOG_INF("USB_DC_CONNECTED");
      break;
    case USB_DC_CONFIGURED:
      LOG_INF("USB_DC_CONFIGURED");
      break;
    case USB_DC_DISCONNECTED:
      LOG_INF("USB_DC_DISCONNECTED");
      break;
    case USB_DC_SUSPEND:
      LOG_INF("USB_DC_SUSPEND");
      suspend_timestamp.reset(k_uptime_get());
      break;
    case USB_DC_RESUME:
      LOG_INF("USB_DC_RESUME");
#if PL_USB_OUTPUT_COUNT > 1
      // We may have resumed after having failed to probe.
      // Retry from the beginning if it's been more than a second.
      if (suspend_timestamp) {
        if (!hid->ProbeResult()) {
          s64_t now = k_uptime_get();
          if (now - *suspend_timestamp > 1000) {
            LOG_INF("resumed after suspend, retrying probe");
            passinglink::usb_reset_probe();
            reboot();
          }
        }
        suspend_timestamp.reset();
      }
#endif
      break;
    case USB_DC_INTERFACE:
      LOG_INF("USB_DC_INTERFACE");
      break;
    case USB_DC_SET_HALT:
      LOG_INF("USB_DC_SET_HALT");
      break;
    case USB_DC_CLEAR_HALT:
      LOG_INF("USB_DC_CLEAR_HALT(0x%02x)", *param);
      hid->ClearHalt(*param);
      if (*param & 0x80) {
        LOG_WRN("halt cleared on input descriptor, queueing write");
        submit_write();
      }
      break;
    case USB_DC_SOF:
      LOG_INF("USB_DC_SOF");
      break;
    case USB_DC_UNKNOWN:
      LOG_INF("USB_DC_UNKNOWN");
      break;
    default:
      LOG_ERR("invalid USB DC status code: %d", status);
      break;
  }
}

static bool decode_hid_report_value(u16_t value, optional<HidReportType>* out_type, u8_t* out_id) {
  u8_t type = value >> 8;
  u8_t id = value & 0xFFu;
  switch (type) {
    case 0:
      out_type->reset();
      break;
    case 1:
      out_type->reset(HidReportType::Input);
      break;
    case 2:
      out_type->reset(HidReportType::Output);
      break;
    case 3:
      out_type->reset(HidReportType::Feature);
      break;
    default:
      LOG_ERR("invalid HID report type: %d", type);
      return false;
  }
  *out_id = id;
  return true;
}

static const struct hid_ops ops = {
    .get_report =
        [](struct usb_setup_packet* setup, s32_t* len, u8_t** data) {
          optional<HidReportType> report_type;
          u8_t report_id;
          if (!decode_hid_report_value(setup->wValue, &report_type, &report_id)) {
            LOG_ERR("failed to decode HID report value: %u", setup->wValue);
            return -1;
          }

          int result = hid->GetReport(report_type, report_id, span<u8_t>(*data, *len));
          if (result != -1) {
            *len = result;
            return 0;
          }
          return -1;
        },
    .get_idle =
        [](struct usb_setup_packet* setup, s32_t* len, u8_t** data) {
          LOG_ERR("Get_Idle unimplemented");
          return -1;
        },
    .get_protocol =
        [](struct usb_setup_packet* setup, s32_t* len, u8_t** data) {
          LOG_ERR("Get_Protocol unimplemented");
          return -1;
        },
    .set_report =
        [](struct usb_setup_packet* setup, s32_t* len, u8_t** data) {
          optional<HidReportType> report_type;
          u8_t report_id;
          if (!decode_hid_report_value(setup->wValue, &report_type, &report_id)) {
            return -1;
          }

          bool result = hid->SetReport(report_type, report_id, span<u8_t>(*data, *len));
          return result ? 0 : -1;
        },
    .set_idle =
        [](struct usb_setup_packet* setup, s32_t* len, u8_t** data) {
          submit_write();
          return 0;
        },
    .set_protocol =
        [](struct usb_setup_packet* setup, s32_t* len, u8_t** data) {
          LOG_ERR("Set_Protocol unimplemented");
          return -1;
        },
    .protocol_change =
        [](u8_t protocol) {
          const char* type = "<invalid>";
          switch (protocol) {
            case HID_PROTOCOL_BOOT:
              type = "boot";
              break;
            case HID_PROTOCOL_REPORT:
              type = "report";
              break;
          }
          LOG_WRN("Protocol change: %s", type);
        },
    .on_idle =
        [](u16_t report_id) {
          // TODO: Write reports to interrupt endpoint.
          LOG_ERR("USB HID report 0x%02x idle", report_id);
        },
    .int_in_ready = []() { submit_write(); },
    .int_out_ready =
        []() {
          u8_t input_buf[64];
          size_t bytes_read;
          int rc = hid_int_ep_read(usb_hid_device, input_buf, sizeof(input_buf), &bytes_read);
          if (rc != 0) {
            LOG_ERR("failed to read from interrupt out endpoint: rc = %d", rc);
            return;
          }
          hid->InterruptOut(span<u8_t>(input_buf, bytes_read));
        },
};

namespace passinglink {

int usb_hid_init(Hid* hid_impl) {
  k_delayed_work_init(&delayed_write_work, write_report);

  hid = hid_impl;

  LOG_INF("initializing USB HID as %s", hid->Name());
  int rc = hid->Init();
  if (rc != 0) {
    LOG_ERR("HID initialization failed: rc = %d", rc);
    return rc;
  }

  usb_hid_device = device_get_binding("HID_0");
  if (usb_hid_device == NULL) {
    LOG_ERR("failed to acquire USB HID device");
    return -ENODEV;
  }

  span<const u8_t> report_descriptor = hid->ReportDescriptor();
  usb_hid_register_device(usb_hid_device, report_descriptor.data(), report_descriptor.size(), &ops);

  rc = usb_hid_init(usb_hid_device);
  if (rc != 0) {
    LOG_ERR("failed to initialize USB hid: rc = %d", rc);
    return rc;
  }

  rc = usb_enable(usb_status_cb);
  if (rc != 0) {
    LOG_ERR("failed to initialize USB");
    return rc;
  }

  return 0;
}

void usb_hid_uninit() {
  k_delayed_work_cancel(&delayed_write_work);
  usb_disable();
  usb_hid_unregister_device(usb_hid_device);
}

}  // namespace passinglink
