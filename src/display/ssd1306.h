#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

// Columns of text
#define DISPLAY_WIDTH 21

// Rows of text
#define DISPLAY_ROWS 3

bool ssd1306_init();

// Display primitives
void display_set_line(size_t row_idx, const char* line);
void display_draw_logo();
void display_blit();

#if defined(__cplusplus)
}
#endif
