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
  virtual void Deinit() {}

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
