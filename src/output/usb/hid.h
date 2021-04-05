#pragma once

#include <sys/types.h>

#include "types.h"

enum class HidReportType {
  Input,
  Output,
  Feature,
};

enum class PLReportId : uint8_t {
  Version = 0x40,

  // Reboot:
  // struct {
  //  uint8_t reboot_type; // 1 for regular reboot, 2 for DFU
  // };
  Reboot = 0x41,

  // Read the version of the provisioning protocol.
  // 0 if unsupported.
  ProvisioningVersion = 0x42,

  // Write part of ProvisioningData into a temporary.
  // struct {
  //   uint8_t offset; // in multiples of 62 bytes
  //   uint8_t data[62];
  // };
  WriteProvisioning = 0x43,

  // Flush accumulated writes from WriteProvisioning to flash.
  // struct {
  //   uint32_t magic; // 0x1209214c
  // };
  FlushProvisioning = 0x44,

  PS4Auth = 0xf0,
};

#define PL_HID_REPORT_DESCRIPTOR                             \
  0x06, 0x42, 0xFF, /* Usage Page (Vendor Defined 0xFF42) */ \
    0x09, 0x01,     /* Usage (0x01) */                       \
    0xA1, 0x01,     /* Collection (Application) */           \
    0x85, 0x40,     /*   Report ID (64) */                   \
    0xB1, 0x02,     /*   Feature(...) */                     \
    0x85, 0x41,     /*   Report ID (65) */                   \
    0xB1, 0x02,     /*   Feature(...) */                     \
    0x85, 0x42,     /*   Report ID (66) */                   \
    0xB1, 0x02,     /*   Feature(...) */                     \
    0x85, 0x43,     /*   Report ID (67) */                   \
    0xB1, 0x02,     /*   Feature(...) */                     \
    0x85, 0x44,     /*   Report ID (68) */                   \
    0xB1, 0x02,     /*   Feature(...) */                     \
    0xC0,           /* End Collection */                     \

class Hid {
 public:
  virtual const char* Name() const = 0;
  virtual int Init() { return 0; }

  virtual span<const uint8_t> ReportDescriptor() const = 0;
  virtual optional<ssize_t> GetReport(optional<HidReportType> report_type, uint8_t report_id,
                                      span<uint8_t> buf);
  virtual optional<bool> SetReport(optional<HidReportType> report_type, uint8_t report_id,
                                   span<uint8_t> data);

  virtual void InterruptOut(span<uint8_t> data) {}

  virtual void ClearHalt(uint8_t endpoint) {}

  virtual k_timeout_t ProbeDelay() = 0;
  virtual bool ProbeResult() { return false; }
};

uint32_t usb_hid_get_report_delay_ticks();
void usb_hid_set_report_delay_ticks(uint32_t ticks);

namespace passinglink {
int usb_hid_init(Hid* hid_impl);
void usb_hid_uninit();
}  // namespace passinglink
