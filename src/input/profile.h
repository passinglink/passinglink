#pragma once

#include "input/socd.h"

void input_profile_init();

size_t input_profile_count();

const char* input_profile_get_name(size_t idx);

size_t input_profile_get_active();
void input_profile_activate(size_t idx);

StickOutput::Axis input_profile_socd_x(const RawInputState* in);
StickOutput::Axis input_profile_socd_y(const RawInputState* in);
bool input_profile_parse_menu(const RawInputState* in, StickOutput stick, uint64_t current_tick);

void input_profile_assign_buttons(InputState* out, const RawInputState* in);
