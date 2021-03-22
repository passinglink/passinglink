#pragma once

#include "input/socd.h"

void input_profile_init();

size_t input_profile_count();
const char* input_profile_get_name(size_t idx);

size_t input_profile_get_active();
void input_profile_activate(size_t idx);

void input_profile_parse(InputState* out, const RawInputState* in, uint64_t current_tick);
