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
