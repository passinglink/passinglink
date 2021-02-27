#pragma once

#include <stddef.h>
#include <stdint.h>

#include "output/usb/probe_type.h"

// Columns of text
#define DISPLAY_WIDTH 21

// Rows of text
#define DISPLAY_ROWS 3

void display_init();

// Display primitives
void display_set_line(size_t row_idx, const char* line);
void display_draw_logo();
void display_blit();

void display_update_latency(uint32_t ticks);
void display_set_locked(bool locked);
void display_set_connection_type(bool probing, ProbeType type);
