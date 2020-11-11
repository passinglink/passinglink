#include "opt/gundam.h"

#include <shell/shell.h>

#include "input/queue.h"

#if defined(CONFIG_PASSINGLINK_BT)
#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include "bt/bt.h"
#endif

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

#if defined(CONFIG_SHELL)
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

#if defined(CONFIG_PASSINGLINK_BT)
static struct bt_uuid_128 bt_gundam_svc_uuid = BT_UUID_INIT_128(0x01, 0x28, PL_BT_UUID_PREFIX);
static struct bt_uuid_128 bt_gundam_camera_uuid = BT_UUID_INIT_128(0x02, 0x28, PL_BT_UUID_PREFIX);

static ssize_t bt_gundam_read(struct bt_conn* conn, const struct bt_gatt_attr* attr, void* buf,
                              uint16_t len, uint16_t offset) {
  LOG_INF("bt: read");
  return bt_gatt_attr_read(conn, attr, buf, len, offset, &current_cam, 1);
}

static ssize_t bt_gundam_write(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                               const void* buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (offset > 0 || len != 1) {
    LOG_ERR("bt: write: invalid length (len = %d, offset = %d)", len, offset);
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }

  uint8_t x = static_cast<const uint8_t*>(buf)[0];
  LOG_INF("bt: setting camera to %d", x);
  opt::gundam::set_cam(x, true);
  return len;
}

// clang-format off
BT_GATT_SERVICE_DEFINE(bt_gundam_svc,
  BT_GATT_PRIMARY_SERVICE(&bt_gundam_svc_uuid),
  BT_GATT_CHARACTERISTIC(
    &bt_gundam_camera_uuid.uuid,
    BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
    bt_gundam_read,
    bt_gundam_write,
    &current_cam
  ),
);
// clang-format on
#endif  // defined(CONFIG_PASSINGLINK_BT)
#endif  // defined(CONFIG_PASSINGLINK_OPT_GUNDAM_CAMERA)
