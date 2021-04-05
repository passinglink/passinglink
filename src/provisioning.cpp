#include "provisioning.h"

#include <zephyr.h>

#include <storage/flash_map.h>

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(provisioning);

static const ProvisioningData* pd;

void provisioning_init() {
#if !FLASH_AREA_LABEL_EXISTS(provisioning)
  LOG_WRN("no provisioning partition defined");
#else
  size_t offset = FLASH_AREA_OFFSET(provisioning);
  size_t size = FLASH_AREA_SIZE(provisioning);
  LOG_INF("provisioning partition defined at 0x%08zx (len = 0x%08zx)", offset, size);

  ProvisioningData* p = reinterpret_cast<ProvisioningData*>(offset);
  if (p->version != ProvisioningVersion::V1) {
    LOG_WRN("invalid magic number for provisioning partition: 0x%08x)",
            static_cast<uint32_t>(p->version));
    return;
  }

  if (strnlen(p->board_name, sizeof(p->board_name)) >= sizeof(p->board_name)) {
    LOG_ERR("non-terminated board name in provisioning partition");
    return;
  }

  LOG_INF("provisioning partition found: board = %s", log_strdup(p->board_name));
  pd = p;
#endif
}

const ProvisioningData* provisioning_data_get() {
  return pd;
}

#if defined(CONFIG_PASSINGLINK_RUNTIME_PROVISIONING)
// TODO: Write directly into flash instead of to a temporary first?
static char provisioning_buffer[4096];
static size_t provisioning_length;

bool provisioning_write(const void* data, size_t length, size_t offset) {
  LOG_INF("provisioning_write: [%zu, %zu)", offset, offset + length);

  if (length + offset > sizeof(provisioning_buffer)) {
    LOG_ERR("overflow, aborting");
    return false;
  }
  memcpy(provisioning_buffer + offset, static_cast<const char*>(data), length);
  provisioning_length = max(provisioning_length, offset + length);
  return true;
}

bool provisioning_flush() {
  LOG_INF("provisioning_flush: %zu bytes", provisioning_length);
  const struct flash_area* flash_area;
  if (flash_area_open(FLASH_AREA_ID(provisioning), &flash_area) != 0) {
    LOG_ERR("provisioning_flush: failed to open flash area");
    return false;
  }
  if (flash_area_erase(flash_area, 0, FLASH_AREA_SIZE(provisioning)) != 0) {
    LOG_ERR("provisioning_flush: failed to erase flash area");
    return false;
  }
  if (flash_area_write(flash_area, 0, &provisioning_buffer, provisioning_length) != 0) {
    LOG_ERR("provisioning_flush: failed to write flash area");
    return false;
  }
  return true;
}
#endif
