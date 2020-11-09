#include "opt/gundam.h"

#include <shell/shell.h>

#include "input/queue.h"

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(gundam);

#if defined(CONFIG_PASSINGLINK_OPT_GUNDAM_CAMERA)

static uint8_t current_cam = 1;

namespace opt {
namespace gundam {

void reset_cam(uint8_t x) {
  current_cam = x;
}

void adjust_cam(int8_t offset, bool record) {
  size_t count = 0;
  bool up = false;

  if (record) {
    current_cam += offset;
  }

  if (offset == 0) {
    return;
  } else if (offset > 0) {
    count = offset;
    up = true;
  } else {
    count = -offset;
    up = false;
  }

  if (offset != 0) {
    InputQueue* head = input_queue_alloc();
    if (!head) {
      LOG_ERR("failed to allocate!");
      return;
    }

    InputQueue* cur = head;
    for (size_t i = 0; i < count; ++i) {
      cur->state = {};
      if (up) {
        cur->state.stick_up = 1;
      } else {
        cur->state.stick_down = 1;
      }
      cur->delay = K_USEC(16'666);
      cur = input_queue_append(cur);
      if (!cur) {
        LOG_ERR("failed to allocate!");
        input_queue_free(head);
        return;
      }

      cur->state = {};
      cur->delay = K_USEC(16'666);
      cur = input_queue_append(cur);
      if (!cur) {
        LOG_ERR("failed to allocate!");
        input_queue_free(head);
        return;
      }
    }

    cur->state = {};
    cur->delay = K_USEC(0);
    input_queue_set_active(head, true);
  }
}

void set_cam(uint8_t x, bool record) {
  adjust_cam(x - current_cam, record);
}

}  // namespace gundam
}  // namespace opt

static int cmd_cam(const struct shell* shell, size_t argc, char** argv) {
  if (argc != 2) {
    shell_print(shell, "usage: cam [next | prev | reset | NUMBER]");
    return 0;
  }

  size_t count = 0;
  bool up = false;
  if (strcmp(argv[1], "reset") == 0) {
    opt::gundam::reset_cam(1);
  } else if (strcmp(argv[1], "next") == 0) {
    opt::gundam::adjust_cam(1, false);
  } else if (strcmp(argv[1], "prev") == 0) {
    opt::gundam::adjust_cam(-1, false);
  } else {
    int x = argv[1][0] - '0';
    if (x < 0 || x > 9) {
      shell_print(shell, "usage: cam [next | prev | reset | NUMBER]");
      return 0;
    }
    opt::gundam::set_cam(x, true);
  }

  return 0;
}

SHELL_CMD_REGISTER(cam, NULL, "Gundam spectator camera control", cmd_cam);

#endif
