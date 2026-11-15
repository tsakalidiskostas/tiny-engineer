#pragma once

#include <cstdint>

// Reads PCA9685 MODE1 once a second. A failed read means the i2c-ng driver
// is stuck (ESP_ERR_INVALID_STATE on every write); restart the bus.
void updateI2cRecovery(uint32_t nowMs);
