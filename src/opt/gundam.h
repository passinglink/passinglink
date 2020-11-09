#pragma once

#include <stdint.h>

#if defined(CONFIG_PASSINGLINK_OPT_GUNDAM_CAMERA)

namespace opt {
namespace gundam {

void reset_cam(uint8_t x);
void adjust_cam(int8_t offset, bool record);
void set_cam(uint8_t x, bool record);

}  // namespace gundam
}  // namespace opt

#endif
