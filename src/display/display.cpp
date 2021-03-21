#include "display/display.h"

#include <stdio.h>

#include "arch.h"
#include "display/menu.h"
#include "display/ssd1306.h"
#include "types.h"
#include "util.h"

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(display);

static bool status_locked;
static bool status_probing;
static optional<uint32_t> status_latency;
static ProbeType status_probe_type;

static void display_draw_status_line() {
  char buf[DISPLAY_WIDTH + 1];
  static_assert(DISPLAY_WIDTH == 21);
  char* p = buf;

  memset(buf, ' ', sizeof(buf) - 1);

  switch (status_probe_type) {
    case ProbeType::NX:
      p += copy_text(buf, "Switch");
      break;

    case ProbeType::PS3:
      p += copy_text(buf, "PS3");
      break;

    case ProbeType::PS4:
      p += copy_text(buf, "PS4");
      break;
  }

  if (status_probing) {
    p[0] = '?';
  }

  if (status_probe_type == ProbeType::NX) {
    p = buf + 8;
    if (!status_probing) {
      ++p;
    }
  } else {
    p = buf + 7;
  }

  if (status_locked) {
    memcpy(p, "LOCKED", strlen("LOCKED"));
  }

  if (!status_latency) {
    snprintf(buf + 15, 7, " ???us");
  } else {
    snprintf(buf + 15, 7, "%4uus", *status_latency);
  }

  buf[DISPLAY_WIDTH] = '\0';
  display_set_line(DISPLAY_ROWS, buf);
}

void display_update_latency(uint32_t us) {
  status_latency.reset(us);
  display_draw_status_line();
  display_blit();
}

void display_set_locked(bool locked) {
  status_locked = locked;
  display_draw_status_line();
  display_blit();
}

void display_set_connection_type(bool probing, ProbeType type) {
  LOG_INF("display_set_connection_type: probing = %d", probing);
  status_probing = probing;
  status_probe_type = type;
  display_draw_status_line();
  display_blit();
}

void display_init() {
  status_locked = false;
  status_probing = true;
  status_probe_type = ProbeType::NX;

  ssd1306_init();
  menu_init();

  display_draw_logo();
  display_draw_status_line();
  display_blit();
}
