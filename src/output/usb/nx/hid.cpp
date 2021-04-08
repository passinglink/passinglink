#include "output/usb/nx/hid.h"

#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <usb/usb_device.h>
#include <zephyr.h>

#include <logging/log.h>
#include <sys/crc.h>

#include "input/input.h"
#include "output/usb/hid.h"
#include "types.h"

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(NXHid);

// clang-format off
const uint8_t kNXReportDescriptor[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x05,        // Usage (Game Pad)
  0xA1, 0x01,        // Collection (Application)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x35, 0x00,        //   Physical Minimum (0)
  0x45, 0x01,        //   Physical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x10,        //   Report Count (16)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x10,        //   Usage Maximum (0x10)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
  0x25, 0x07,        //   Logical Maximum (7)
  0x46, 0x3B, 0x01,  //   Physical Maximum (315)
  0x75, 0x04,        //   Report Size (4)
  0x95, 0x01,        //   Report Count (1)
  0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
  0x09, 0x39,        //   Usage (Hat switch)
  0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
  0x65, 0x00,        //   Unit (None)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x46, 0xFF, 0x00,  //   Physical Maximum (255)
  0x09, 0x30,        //   Usage (X)
  0x09, 0x31,        //   Usage (Y)
  0x09, 0x32,        //   Usage (Z)
  0x09, 0x35,        //   Usage (Rz)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
  0x09, 0x20,        //   Usage (0x20)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x0A, 0x21, 0x26,  //   Usage (0x2621)
  0x95, 0x08,        //   Report Count (8)
  0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0xC0,              // End Collection

  PL_HID_REPORT_DESCRIPTOR
};
// clang-format on

struct __attribute__((packed)) OutputReport {
  uint16_t button_west : 1;
  uint16_t button_south : 1;
  uint16_t button_east : 1;
  uint16_t button_north : 1;
  uint16_t button_l1 : 1;
  uint16_t button_r1 : 1;
  uint16_t button_l2 : 1;
  uint16_t button_r2 : 1;
  uint16_t button_select : 1;
  uint16_t button_start : 1;
  uint16_t button_l3 : 1;
  uint16_t button_r3 : 1;
  uint16_t button_home : 1;
  uint16_t button_touchpad : 1;
  uint16_t button_15 : 1;
  uint16_t button_16 : 1;
  uint8_t dpad : 4;
  uint8_t unknown_1 : 4;
  uint8_t left_stick_x;
  uint8_t left_stick_y;
  uint8_t right_stick_x;
  uint8_t right_stick_y;
  uint8_t unknown_2;
};

static_assert(sizeof(OutputReport) == 8);

int NXHid::Init() {
  usb_set_vendor_id(0x0f0d);
  usb_set_product_id(0x0092);
  return 0;
}

void NXHid::Deinit() {
  usb_set_vendor_id(CONFIG_USB_DEVICE_VID);
  usb_set_product_id(CONFIG_USB_DEVICE_PID);
}

span<const uint8_t> NXHid::ReportDescriptor() const {
  return span<const uint8_t>(reinterpret_cast<const uint8_t*>(kNXReportDescriptor),
                             sizeof(kNXReportDescriptor));
}

ssize_t NXHid::GetFeatureReport(uint8_t report_id, span<uint8_t> buf) {
  buf[0] = report_id;
  LOG_ERR("GetFeatureReport called for unknown report 0x%02X", report_id);
  return -1;
}

ssize_t NXHid::GetInputReport(uint8_t report_id, span<uint8_t> buf) {
  switch (report_id) {
    case 0x01: {
      if (buf.size() != 64) {
        LOG_ERR("GetInputReport: invalid buffer size %zu", buf.size());
        return -1;
      }

      InputState input;
      if (!input_get_state(&input)) {
        LOG_ERR("failed to get InputState");
        return -1;
      }

      OutputReport output = {};
      output.left_stick_x = input.left_stick_x;
      output.left_stick_y = input.left_stick_y;
      output.right_stick_x = input.right_stick_x;
      output.right_stick_y = input.right_stick_y;
      switch (static_cast<StickState>(input.dpad)) {
        case StickState::North:
          output.dpad = 0;
          break;
        case StickState::NorthEast:
          output.dpad = 1;
          break;
        case StickState::East:
          output.dpad = 2;
          break;
        case StickState::SouthEast:
          output.dpad = 3;
          break;
        case StickState::South:
          output.dpad = 4;
          break;
        case StickState::SouthWest:
          output.dpad = 5;
          break;
        case StickState::West:
          output.dpad = 6;
          break;
        case StickState::NorthWest:
          output.dpad = 7;
          break;
        case StickState::Neutral:
          output.dpad = 8;
          break;
        default:
          LOG_ERR("invalid stick state: %d", static_cast<int>(input.dpad));
          return -1;
      }

      output.button_north = input.button_north;
      output.button_east = input.button_east;
      output.button_south = input.button_south;
      output.button_west = input.button_west;
      output.button_l1 = input.button_l1;
      output.button_l2 = input.button_l2;
      output.button_l3 = input.button_l3;
      output.button_r1 = input.button_r1;
      output.button_r2 = input.button_r2;
      output.button_r3 = input.button_r3;
      output.button_select = input.button_select;
      output.button_start = input.button_start;
      output.button_home = input.button_home;
      output.button_touchpad = input.button_touchpad;

      memcpy(buf.data(), &output, sizeof(output));
      return sizeof(output);
    }

    default:
      LOG_ERR("GetInputReport called for unknown report 0x%02X", report_id);
      return -1;
  }
}

ssize_t NXHid::GetReport(optional<HidReportType> report_type, uint8_t report_id,
                           span<uint8_t> buf) {
  LOG_DBG("GetReport(0x%02X)", report_id);
  if (!report_type) {
    LOG_ERR("ignoring GetReport without a report type");
    return -1;
  } else if (*report_type == HidReportType::Feature) {
    return GetFeatureReport(report_id, buf);
  } else if (*report_type == HidReportType::Input) {
    return GetInputReport(report_id, buf);
  } else if (*report_type == HidReportType::Output) {
    LOG_ERR("ignoring GetReport on output report %d", static_cast<int>(*report_type));
    return -1;
  }
  return -1;
}
