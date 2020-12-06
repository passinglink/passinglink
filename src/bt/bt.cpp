#include "bt/bt.h"

#include <zephyr.h>

#if defined(CONFIG_PASSINGLINK_BT)

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#include <logging/log.h>

#include "input/input.h"
#include "opt/gundam.h"
#include "version.h"

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(bt);

static const struct bt_le_adv_param pl_bt_adv_params =
    BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
                         BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, nullptr);

static const struct bt_data pl_bt_adv_data[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, 0x00, 0x00, PL_BT_UUID_PREFIX),
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static struct bt_conn_cb connection_cbs = {
    .connected =
        [](struct bt_conn* conn, uint8_t err) {
          if (err) {
            LOG_ERR("connection failed (err 0x%02x)", err);
          } else {
            LOG_INF("connection succeeded");
            struct bt_le_conn_param param = {
              .interval_min = 6,
              .interval_max = 6,
              .latency = 0,
              .timeout = 3200,
            };
            if (bt_conn_le_param_update(conn, &param) != 0) {
              LOG_WRN("failed to update bluetooth connection parameters");
            }
          }
        },
    .disconnected =
        [](struct bt_conn* conn, uint8_t reason) {
          LOG_INF("connection terminated (reason 0x%02x)", reason);
        },
};
#pragma GCC diagnostic pop

void bluetooth_init() {
  int err = bt_enable(nullptr);
  if (err != 0) {
    LOG_ERR("initialization failed: error = %d", err);
    return;
  }

  err = bt_set_name("Passing Link");
  if (err != 0) {
    LOG_ERR("bt_set_name failed: error = %d", err);
    return;
  }

#if defined(CONFIG_PASSINGLINK_BT_AUTHENTICATION)
  // XXX: If the passkey is set to "1234", pairing seems to fail unless "001234" is used.
  static_assert(CONFIG_PASSINGLINK_BT_PAIRING_KEY >= 100000, "BT pairing key must be 6 digits");
  static_assert(CONFIG_PASSINGLINK_BT_PAIRING_KEY <= 999999, "BT pairing key must be 6 digits");
  err = bt_passkey_set(CONFIG_PASSINGLINK_BT_PAIRING_KEY);
  if (err != 0) {
    LOG_ERR("bt_passkey_set failed: error = %d", err);
    return;
  } else {
    LOG_INF("Bluetooth passkey set to %d", CONFIG_PASSINGLINK_BT_PAIRING_KEY);
  }
#else
  LOG_INF("Bluetooth authentication disabled");
#endif

  bt_conn_cb_register(&connection_cbs);

  err = bt_le_adv_start(&pl_bt_adv_params, pl_bt_adv_data, ARRAY_SIZE(pl_bt_adv_data), nullptr, 0);
  if (err) {
    LOG_ERR("advertising failed to start: error = %d", err);
    return;
  }
}

static struct bt_uuid_128 bt_version_svc_uuid = BT_UUID_INIT_128(0x00, 0x00, PL_BT_UUID_PREFIX);

static struct bt_uuid_128 bt_version_str_uuid = BT_UUID_INIT_128(0x01, 0x00, PL_BT_UUID_PREFIX);
static struct bt_uuid_128 bt_version_branch_uuid = BT_UUID_INIT_128(0x02, 0x00, PL_BT_UUID_PREFIX);
static struct bt_uuid_128 bt_version_commit_uuid = BT_UUID_INIT_128(0x03, 0x00, PL_BT_UUID_PREFIX);

static ssize_t bt_version_str_read(struct bt_conn* conn, const struct bt_gatt_attr* attr, void* buf,
                               uint16_t len, uint16_t offset) {
  const char* version = version_string();
  return bt_gatt_attr_read(conn, attr, buf, len, offset, version, strlen(version));
}

static ssize_t bt_version_branch_read(struct bt_conn* conn, const struct bt_gatt_attr* attr, void* buf,
                               uint16_t len, uint16_t offset) {
  return bt_gatt_attr_read(conn, attr, buf, len, offset, git_branch, strlen(git_branch));
}

static ssize_t bt_version_commit_read(struct bt_conn* conn, const struct bt_gatt_attr* attr, void* buf,
                               uint16_t len, uint16_t offset) {
  return bt_gatt_attr_read(conn, attr, buf, len, offset, git_commit, strlen(git_commit));
}

// clang-format off
BT_GATT_SERVICE_DEFINE(bt_version_svc,
  BT_GATT_PRIMARY_SERVICE(&bt_version_svc_uuid),
  BT_GATT_CHARACTERISTIC(
    &bt_version_str_uuid.uuid,
    BT_GATT_CHRC_READ,
#if CONFIG_PASSINGLINK_BT_AUTHENTICATION
    BT_GATT_PERM_READ_ENCRYPT,
#else
    BT_GATT_PERM_READ,
#endif
    bt_version_str_read,
    nullptr,
    nullptr
  ),
  BT_GATT_CHARACTERISTIC(
    &bt_version_branch_uuid.uuid,
    BT_GATT_CHRC_READ,
#if CONFIG_PASSINGLINK_BT_AUTHENTICATION
    BT_GATT_PERM_READ_ENCRYPT,
#else
    BT_GATT_PERM_READ,
#endif
    bt_version_branch_read,
    nullptr,
    nullptr
  ),
  BT_GATT_CHARACTERISTIC(
    &bt_version_commit_uuid.uuid,
    BT_GATT_CHRC_READ,
#if CONFIG_PASSINGLINK_BT_AUTHENTICATION
    BT_GATT_PERM_READ_ENCRYPT,
#else
    BT_GATT_PERM_READ,
#endif
    bt_version_commit_read,
    nullptr,
    nullptr
  ),
);

#if defined(CONFIG_PASSINGLINK_BT_INPUT)
static struct bt_uuid_128 bt_input_svc_uuid = BT_UUID_INIT_128(0x00, 0x01, PL_BT_UUID_PREFIX);
static struct bt_uuid_128 bt_input_attr_uuid = BT_UUID_INIT_128(0x01, 0x01, PL_BT_UUID_PREFIX);

static ssize_t bt_input_read(struct bt_conn* conn, const struct bt_gatt_attr* attr, void* buf,
                             uint16_t len, uint16_t offset) {
  LOG_INF("input: read");
  RawInputState input;
  if (!input_get_raw_state(&input)) {
    return BT_GATT_ERR(BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
  }
  return bt_gatt_attr_read(conn, attr, buf, len, offset, &input, sizeof(input));
}

static ssize_t bt_input_write(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                              const void* buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (offset > 0 || len != sizeof(RawInputState)) {
    LOG_ERR("input: write: invalid length (len = %d, offset = %d)", len, offset);
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }

  RawInputState state;
  memcpy(&state, buf, sizeof(state));

  LOG_INF("input: write");
#define PL_GPIO(id, name) \
  if (state.name) {       \
    LOG_INF(#name);       \
  }
  PL_GPIOS()
#undef PL_GPIO

  input_set_raw_state(&state);
  return len;
}

// clang-format off
BT_GATT_SERVICE_DEFINE(bt_input_svc,
  BT_GATT_PRIMARY_SERVICE(&bt_input_svc_uuid),
  BT_GATT_CHARACTERISTIC(
    &bt_input_attr_uuid.uuid,
    BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
#if CONFIG_PASSINGLINK_BT_AUTHENTICATION
    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
#else
    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
#endif
    bt_input_read,
    bt_input_write,
    nullptr
  ),
);
// clang-format on
#endif  // defined(CONFIG_PASSINGLINK_BT_INPUT)
#endif  // defined(CONFIG_PASSINGLINK_BT)
