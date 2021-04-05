#pragma once

#include <sys/types.h>

#include <kernel.h>

#include "output/usb/hid.h"
#include "types.h"

class PS3Hid : public Hid {
 public:
  virtual const char* Name() const override final { return "PS3"; }

  virtual span<const uint8_t> ReportDescriptor() const override final;
  ssize_t GetFeatureReport(uint8_t report_id, span<uint8_t> buf);
  ssize_t GetInputReport(uint8_t report_id, span<uint8_t> buf);
  virtual optional<ssize_t> GetReport(optional<HidReportType> report_type, uint8_t report_id,
                                      span<uint8_t> buf) override final;

  virtual void InterruptOut(span<uint8_t> buf) override final;

  virtual k_timeout_t ProbeDelay() override final { return K_MSEC(1000); }
  virtual bool ProbeResult() override final { return controller_number_ != 0xFF; }

 private:
  uint8_t controller_number_ = 0xFF;
};
