#pragma once

#include <sys/types.h>

#include "types.h"

enum class HidReportType {
  Input,
  Output,
  Feature,
};

class Hid {
 public:
  virtual const char* Name() const = 0;
  virtual int Init() { return 0; }

  virtual span<const uint8_t> ReportDescriptor() const = 0;
  virtual ssize_t GetReport(optional<HidReportType> report_type, uint8_t report_id,
                            span<uint8_t> buf) = 0;
  virtual bool SetReport(optional<HidReportType> report_type, uint8_t report_id,
                         span<uint8_t> data) {
    return false;
  }

  virtual void InterruptOut(span<uint8_t> data) {}

  virtual void ClearHalt(uint8_t endpoint) {}

  // Delay to wait in ticks before checking whether the Hid is successful.
  virtual uint32_t ProbeDelay() { return 0; }

  virtual bool ProbeResult() { return false; }
};

namespace passinglink {
int usb_hid_init(Hid* hid_impl);
void usb_hid_uninit();
}  // namespace passinglink
