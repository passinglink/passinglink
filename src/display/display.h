#pragma once

#include <stddef.h>
#include <stdint.h>

#include "display/ssd1306.h"
#include "output/usb/probe_type.h"

void display_init();

void display_update_latency(uint32_t ticks);
void display_set_locked(bool locked);
void display_set_connection_type(bool probing, ProbeType type);
