#pragma once

#include "provisioning_types.h"
#include "types.h"

void provisioning_init();
const ProvisioningData* provisioning_data_get();

bool provisioning_write(const void* data, size_t length, size_t offset);
bool provisioning_flush();
