#pragma once

#include <sys/types.h>

#include <kernel.h>

#include "output/usb/hid.h"
#include "types.h"

namespace ps3 {

class Hid : public ::Hid {
 public:
  virtual const char* Name() const override final { return "PS3"; }

  virtual span<const u8_t> ReportDescriptor() const override final;
  ssize_t GetFeatureReport(u8_t report_id, span<u8_t> buf);
  ssize_t GetInputReport(u8_t report_id, span<u8_t> buf);
  virtual ssize_t GetReport(optional<HidReportType> report_type, u8_t report_id,
                            span<u8_t> buf) override final;

  virtual void InterruptOut(span<u8_t> buf) override final;

  virtual uint32_t ProbeDelay() override final { return k_ms_to_ticks_ceil32(1000); }
  virtual bool ProbeResult() override final { return controller_number_ != 0xFF; }

 private:
  uint8_t controller_number_ = 0xFF;
};

}  // namespace ps3
