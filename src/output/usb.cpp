#include <zephyr.h>

#include <init.h>
#include <logging/log.h>

#include <usb/class/usb_hid.h>
#include <usb/usb_device.h>

#include "output/hid.h"
#include "output/output.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(hid);

static Hid* hid;
static struct device* usb_hid_device;

static void write_report() {
  u8_t report_buf[64];
  ssize_t rc = hid->GetReport(HidReportType::Input, 1, span(report_buf, sizeof(report_buf)));
  if (rc < 0) {
    return;
  }

  size_t bytes_written = 0;
  rc = hid_int_ep_write(usb_hid_device, report_buf, rc, &bytes_written);
  if (rc < 0) {
    LOG_ERR("USB write failed: rc = %d", rc);
  }
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
      write_report();
      break;
    case USB_DC_DISCONNECTED:
      LOG_INF("USB_DC_DISCONNECTED");
      break;
    case USB_DC_SUSPEND:
      LOG_INF("USB_DC_SUSPEND");
      break;
    case USB_DC_RESUME:
      LOG_INF("USB_DC_RESUME");
      break;
    case USB_DC_INTERFACE:
      LOG_INF("USB_DC_INTERFACE");
      break;
    case USB_DC_SET_HALT:
      LOG_INF("USB_DC_SET_HALT");
      break;
    case USB_DC_CLEAR_HALT:
      LOG_INF("USB_DC_CLEAR_HALT");
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
          LOG_ERR("Set_Idle unimplemented");
          return -1;
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
    .int_in_ready = []() { write_report(); },
};

int usb_hid_init(Hid* hid_impl) {
  int rc = usb_enable(usb_status_cb);
  if (rc != 0) {
    LOG_ERR("failed to initialize USB");
    return rc;
  }

  hid = hid_impl;

  LOG_INF("initializing USB HID as %s", hid->Name());

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
  return 0;
}
