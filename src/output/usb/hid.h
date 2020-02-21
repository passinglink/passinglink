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

  virtual span<const u8_t> ReportDescriptor() const = 0;
  virtual ssize_t GetReport(optional<HidReportType> report_type, u8_t report_id,
                            span<u8_t> buf) = 0;
  virtual bool SetReport(optional<HidReportType> report_type, u8_t report_id, span<u8_t> data) {
    return false;
  }
};

namespace passinglink {
int usb_hid_init(Hid* hid_impl);
}
