#pragma once

#include "input/socd.h"

void input_profile_init();

StickOutput::Axis input_profile_socd_x(const RawInputState* in);
StickOutput::Axis input_profile_socd_y(const RawInputState* in);
bool input_profile_parse_menu(const RawInputState* in, StickOutput stick, uint64_t current_tick);
void input_profile_assign_buttons(InputState* out, const RawInputState* in);
