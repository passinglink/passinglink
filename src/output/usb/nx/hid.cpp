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
const u8_t kNXReportDescriptor[] = {
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
};
// clang-format on

struct __attribute__((packed)) OutputReport {
  u16_t button_west : 1;
  u16_t button_south : 1;
  u16_t button_east : 1;
  u16_t button_north : 1;
  u16_t button_l1 : 1;
  u16_t button_r1 : 1;
  u16_t button_l2 : 1;
  u16_t button_r2 : 1;
  u16_t button_select : 1;
  u16_t button_start : 1;
  u16_t button_l3 : 1;
  u16_t button_r3 : 1;
  u16_t button_home : 1;
  u16_t button_touchpad : 1;
  u16_t button_15 : 1;
  u16_t button_16 : 1;
  u8_t dpad : 4;
  u8_t unknown_1 : 4;
  u8_t left_stick_x;
  u8_t left_stick_y;
  u8_t right_stick_x;
  u8_t right_stick_y;
  u8_t unknown_2;
};

static_assert(sizeof(OutputReport) == 8);

int nx::Hid::Init() {
  usb_set_vendor_id(0x0f0d);
  usb_set_product_id(0x0092);
  return 0;
}

span<const u8_t> nx::Hid::ReportDescriptor() const {
  return span<const u8_t>(reinterpret_cast<const u8_t*>(kNXReportDescriptor),
                          sizeof(kNXReportDescriptor));
}

ssize_t nx::Hid::GetFeatureReport(u8_t report_id, span<u8_t> buf) {
  buf[0] = report_id;
  LOG_ERR("GetFeatureReport called for unknown report 0x%02X", report_id);
  return -1;
}

ssize_t nx::Hid::GetInputReport(u8_t report_id, span<u8_t> buf) {
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
      output.left_stick_x = 127;
      output.left_stick_y = 127;
      output.right_stick_x = 127;
      output.right_stick_y = 127;
      switch (static_cast<StickState>(input.stick_state)) {
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
          LOG_ERR("invalid stick state: %d", input.stick_state);
          return -1;
      }

#define PL_BUTTON_GPIO(name, NAME) output.button_##name = input.button_##name;
      PL_BUTTON_GPIOS()
#undef PL_BUTTON_GPIO

      memcpy(buf.data(), &output, sizeof(output));
      return sizeof(output);
    }

    default:
      LOG_ERR("GetInputReport called for unknown report 0x%02X", report_id);
      return -1;
  }
}

ssize_t nx::Hid::GetReport(optional<HidReportType> report_type, u8_t report_id, span<u8_t> buf) {
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

bool nx::Hid::SetReport(optional<HidReportType> report_type, u8_t report_id, span<u8_t> data) {
  LOG_WRN("SetReport(0x%02X): %zu byte%s", report_id, data.size(), data.size() == 1 ? "" : "s");

  if (!report_type) {
    LOG_ERR("ignoring SetReport without a report type");
    return false;
  } else if (*report_type != HidReportType::Feature) {
    LOG_ERR("ignoring SetReport on non-feature report %d", static_cast<int>(*report_type));
    return false;
  } else {
    LOG_ERR("SetReport called for unknown report 0x%02X", report_id);
    return false;
  }

  return false;
}
