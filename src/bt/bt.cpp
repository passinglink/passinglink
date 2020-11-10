#include "bt/bt.h"

#include <zephyr.h>

#if defined(CONFIG_PASSINGLINK_BT)

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#include <logging/log.h>

#include "opt/gundam.h"

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

  bt_conn_cb_register(&connection_cbs);

  err = bt_le_adv_start(&pl_bt_adv_params, pl_bt_adv_data, ARRAY_SIZE(pl_bt_adv_data), nullptr, 0);
  if (err) {
    LOG_ERR("advertising failed to start: error = %d", err);
    return;
  }
}

#endif  // defined(CONFIG_PASSINGLINK_BT)
