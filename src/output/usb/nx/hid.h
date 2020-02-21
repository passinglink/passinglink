#pragma once

#include <sys/types.h>

#include "output/usb/hid.h"
#include "types.h"

namespace nx {

class Hid : public ::Hid {
 public:
  virtual const char* Name() const override final { return "Switch"; }
  virtual int Init() override final;

  virtual span<const u8_t> ReportDescriptor() const override final;
  ssize_t GetFeatureReport(u8_t report_id, span<u8_t> buf);
  ssize_t GetInputReport(u8_t report_id, span<u8_t> buf);
  virtual ssize_t GetReport(optional<HidReportType> report_type, u8_t report_id,
                            span<u8_t> buf) override final;
  virtual bool SetReport(optional<HidReportType> report_type, u8_t report_id,
                         span<u8_t> data) override final;

 private:
  uint8_t last_report_counter_ = 0;
};

}  // namespace nx
