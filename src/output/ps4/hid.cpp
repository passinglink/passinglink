#include "output/ps4/hid.h"

#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr.h>

#include <logging/log.h>
#include <sys/crc.h>

#include "input/input.h"
#include "output/hid.h"
#include "output/ps4/auth.h"
#include "types.h"

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(PS4Hid);

// clang-format off
const u8_t kPS4ReportDescriptor[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x05,        // Usage (Game Pad)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x09, 0x30,        //   Usage (X)
  0x09, 0x31,        //   Usage (Y)
  0x09, 0x32,        //   Usage (Z)
  0x09, 0x35,        //   Usage (Rz)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

  0x09, 0x39,        //   Usage (Hat switch)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x07,        //   Logical Maximum (7)
  0x35, 0x00,        //   Physical Minimum (0)
  0x46, 0x3B, 0x01,  //   Physical Maximum (315)
  0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
  0x75, 0x04,        //   Report Size (4)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)

  0x65, 0x00,        //   Unit (None)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x0E,        //   Usage Maximum (0x0E)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x0E,        //   Report Count (14)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

  0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
  0x09, 0x20,        //   Usage (0x20)
  0x75, 0x06,        //   Report Size (6)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

  0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
  0x09, 0x33,        //   Usage (Rx)
  0x09, 0x34,        //   Usage (Ry)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x02,        //   Report Count (2)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

  0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
  0x09, 0x21,        //   Usage (0x21)
  0x95, 0x36,        //   Report Count (54)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

  0x85, 0x05,        //   Report ID (5)
  0x09, 0x22,        //   Usage (0x22)
  0x95, 0x1F,        //   Report Count (31)
  0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

  0x85, 0x03,        //   Report ID (3)
  0x0A, 0x21, 0x27,  //   Usage (0x2721)
  0x95, 0x2F,        //   Report Count (47)
  0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0xC0,              // End Collection


  0x06, 0xF0, 0xFF,  // Usage Page (Vendor Defined 0xFFF0)
  0x09, 0x40,        // Usage (0x40)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0xF0,        //   Report ID (-16)
  0x09, 0x47,        //   Usage (0x47)
  0x95, 0x3F,        //   Report Count (63)
  0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0x85, 0xF1,        //   Report ID (-15)
  0x09, 0x48,        //   Usage (0x48)
  0x95, 0x3F,        //   Report Count (63)
  0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0x85, 0xF2,        //   Report ID (-14)
  0x09, 0x49,        //   Usage (0x49)
  0x95, 0x0F,        //   Report Count (15)
  0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0x85, 0xF3,        //   Report ID (-13)
  0x0A, 0x01, 0x47,  //   Usage (0x4701)
  0x95, 0x07,        //   Report Count (7)
  0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0xC0,              // End Collection
};
// clang-format on

struct __attribute__((packed)) OutputReport {
  u8_t report_id;
  u8_t left_stick_x;
  u8_t left_stick_y;
  u8_t right_stick_x;
  u8_t right_stick_y;

  // 4 bits for the d-pad.
  u32_t dpad : 4;

  // 14 bits for buttons.
  u32_t button_west : 1;
  u32_t button_south : 1;
  u32_t button_east : 1;
  u32_t button_north : 1;
  u32_t button_l1 : 1;
  u32_t button_r1 : 1;
  u32_t button_l2 : 1;
  u32_t button_r2 : 1;
  u32_t button_select : 1;
  u32_t button_start : 1;
  u32_t button_l3 : 1;
  u32_t button_r3 : 1;
  u32_t button_home : 1;
  u32_t button_touchpad : 1;

  // 6 bit report counter.
  u32_t report_counter : 6;

  u32_t left_trigger : 8;
  u32_t right_trigger : 8;

  // Trackpad and tilt presumably follows.
  u32_t padding : 24;
  u8_t mystery[51];
};

static_assert(sizeof(OutputReport) == 64);

static bool check_crc(span<u8_t> data) {
  if (data.size() < 4) {
    return false;
  }

  u32_t crc = crc32_ieee(data.data(), data.size() - sizeof(crc));
  return memcmp(&crc, data.data() + (data.size() - sizeof(crc)), sizeof(crc)) == 0;
}

static int write_with_crc(span<u8_t> dst, span<u8_t> src) {
  if (dst.size() < 4 || dst.size() - 4 < src.size()) {
    LOG_ERR("write_with_crc: attempted to write %zu bytes into a %zu byte buffer", src.size(),
            dst.size());
    return -1;
  }

  u32_t crc = crc32_ieee(src.data(), src.size());
  memcpy(dst.data(), src.data(), src.size());
  memcpy(dst.data() + src.size(), &crc, 4);
  return src.size() + 4;
}

span<const u8_t> ps4::Hid::ReportDescriptor() const {
  return span<const u8_t>(reinterpret_cast<const u8_t*>(kPS4ReportDescriptor),
                          sizeof(kPS4ReportDescriptor));
}

ssize_t ps4::Hid::GetFeatureReport(u8_t report_id, span<u8_t> buf) {
  buf[0] = report_id;

  switch (report_id) {
    case 0x03: {
      // Unknown, copied from an actual device.
      static constexpr u8_t output_0x03[] = {
          0x3, 0x21, 0x27, 0x4, 0x40, 0x7, 0x2c, 0x56, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
          0x0, 0x0,  0xd,  0xd, 0x0,  0x0, 0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
          0x0, 0x0,  0x0,  0x0, 0x0,  0x0, 0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

      if (buf.size() != sizeof(output_0x03)) {
        LOG_ERR("GetFeatureReport(0x03) called with unexpected size %zu", buf.size());
        return -1;
      }

      memcpy(buf.data(), output_0x03, buf.size());
      return buf.size();
    }

    case 0xF1: {
      // Get signature of nonce.
#if !defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4_AUTH)
      LOG_ERR("GetFeatureReport(0xF1): auth unavailable");
      return -1;
#else
      auto state = get_auth_state();
      if (state.type != AuthStateType::SendingSignature) {
        LOG_ERR("GetFeatureReport(0xF1) called without signature ready");
        return -1;
      }

      if (buf.size() != 64) {
        LOG_ERR("GetFeatureReport(0xF1) called with unexpected size %zu", buf.size());
        return -1;
      }

      u8_t data[60] = {};
      data[0] = 0xF1;
      data[1] = state.nonce_id;
      data[2] = state.next_part;
      data[3] = 0;

      span<u8_t> output_buffer(data, sizeof(data));
      output_buffer.remove_prefix(4);
      if (!get_next_signature_chunk(output_buffer)) {
        return -1;
      }

      return write_with_crc(buf, span<u8_t>(data, sizeof(data)));
#endif
    }

    case 0xF2: {
      // Get signing state.
#if !defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4_AUTH)
      LOG_ERR("GetFeatureReport(0xF2): auth unavailable");
      return -1;
#else
      u8_t data[12] = {};
      auto state = get_auth_state();
      data[0] = 0xF2;
      data[1] = state.nonce_id;
      data[2] = state.type == AuthStateType::SendingSignature ? 0 : 16;
      return write_with_crc(buf, span<u8_t>(data, sizeof(data)));
#endif
    }

    case 0xF3: {
      static constexpr u8_t output_0xf3[] = {0xf3, 0x0, 0x38, 0x38, 0, 0, 0, 0};
      if (buf.size() != sizeof(output_0xf3)) {
        LOG_ERR("GetFeatureReport(0xf3) called with unexpected size %zu", buf.size());
        return -1;
      }

      memcpy(buf.data(), output_0xf3, buf.size());
      return buf.size();
    }

    default:
      LOG_ERR("GetFeatureReport called for unknown report 0x%02X", report_id);
      return -1;
  }
}

ssize_t ps4::Hid::GetInputReport(u8_t report_id, span<u8_t> buf) {
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
      output.report_id = 0x01;
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
          output.dpad = 15;
          break;
        default:
          LOG_ERR("invalid stick state: %d", input.stick_state);
          return -1;
      }

#define PL_BUTTON_GPIO(name, NAME) output.button_##name = input.button_##name;
      PL_BUTTON_GPIOS()
#undef PL_BUTTON_GPIO

      output.report_counter = last_report_counter_++;

      output.left_trigger = 0;
      output.right_trigger = 0;
      memcpy(buf.data(), &output, buf.size());

      return buf.size();
    }

    default:
      LOG_ERR("GetInputReport called for unknown report 0x%02X", report_id);
      return -1;
  }
}

ssize_t ps4::Hid::GetReport(optional<HidReportType> report_type, u8_t report_id, span<u8_t> buf) {
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

bool ps4::Hid::SetReport(optional<HidReportType> report_type, u8_t report_id, span<u8_t> data) {
  LOG_WRN("SetReport(0x%02X): %zu byte%s", report_id, data.size(),
          data.size() == 1 ? "" : "s");

  if (!report_type) {
    LOG_ERR("ignoring SetReport without a report type");
    return false;
  } else if (*report_type != HidReportType::Feature) {
    LOG_ERR("ignoring SetReport on non-feature report %d", static_cast<int>(*report_type));
    return false;
  }

  switch (report_id) {
    case 0xF0: {
#if !defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4_AUTH)
      LOG_ERR("SetReport(0xF0): auth unavailable");
      return -1;
#else
      // Set nonce.
      if (data.size() != 64) {
        LOG_ERR("nonce set: unexpected packet size %zu", data.size());
        return false;
      }

      if (!check_crc(data)) {
        LOG_ERR("nonce set: crc failed");
        return false;
      }

      u8_t nonce_id = data[1];
      u8_t nonce_part = data[2];

      // 4 byte header.
      data.remove_prefix(4);

      // 4 byte CRC.
      data.remove_suffix(4);

      // 256 byte nonce, with 56 byte packets leaves 24 extra bytes on the last packet.
      if (nonce_part == 4) {
        data.remove_suffix(24);
      }

      return set_nonce(nonce_id, nonce_part, data);
#endif
    }

    default:
      LOG_ERR("SetReport called for unknown report 0x%02X", report_id);
      return false;
  }

  return false;
}
