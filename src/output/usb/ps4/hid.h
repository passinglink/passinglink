#pragma once

#include <sys/types.h>

#include <kernel.h>

#include "output/usb/hid.h"
#include "types.h"

class PS4Hid : public Hid {
 public:
  virtual const char* Name() const override final { return "PS4"; }

  virtual span<const uint8_t> ReportDescriptor() const override final;
  ssize_t GetFeatureReport(uint8_t report_id, span<uint8_t> buf);
  ssize_t GetInputReport(uint8_t report_id, span<uint8_t> buf);
  virtual ssize_t GetReport(optional<HidReportType> report_type, uint8_t report_id,
                            span<uint8_t> buf) override final;
  virtual bool SetReport(optional<HidReportType> report_type, uint8_t report_id,
                         span<uint8_t> data) override final;

  // PS4 Hid is the last, and will always fail, but give it a delay to make the LED visible.
  virtual k_timeout_t ProbeDelay() { return K_MSEC(1000); }

 private:
  uint8_t last_report_counter_ = 0;
};
