#pragma once

#include <sys/types.h>

#include <kernel.h>

#include "output/usb/hid.h"
#include "types.h"

namespace nx {

class Hid : public ::Hid {
 public:
  virtual const char* Name() const override final { return "Switch"; }
  virtual int Init() override final;

  virtual span<const uint8_t> ReportDescriptor() const override final;
  ssize_t GetFeatureReport(uint8_t report_id, span<uint8_t> buf);
  ssize_t GetInputReport(uint8_t report_id, span<uint8_t> buf);
  virtual ssize_t GetReport(optional<HidReportType> report_type, uint8_t report_id,
                            span<uint8_t> buf) override final;
  virtual bool SetReport(optional<HidReportType> report_type, uint8_t report_id,
                         span<uint8_t> data) override final;

  virtual void ClearHalt(uint8_t endpoint) override final {
    if (endpoint & 0x80) {
      input_halt_cleared_ = true;
    } else {
      output_halt_cleared_ = true;
    }
  }

  virtual uint32_t ProbeDelay() override final { return k_ms_to_ticks_ceil32(1000); }
  virtual bool ProbeResult() override final { return input_halt_cleared_ && output_halt_cleared_; }

 private:
  uint8_t last_report_counter_ = 0;
  bool input_halt_cleared_ = false;
  bool output_halt_cleared_ = false;
};

}  // namespace nx
