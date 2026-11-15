#include "hardware/i2c_recovery.h"

#include <Arduino.h>
#include <Wire.h>

#include "pins.h"

namespace {

constexpr uint32_t kCheckMs = 1000;
constexpr uint8_t kPcaMode1 = 0x00;

uint32_t g_lastCheckMs = 0;
uint32_t g_resetCount = 0;

bool pcaReadOk() {
  Wire.beginTransmission(PCA9685_ADDRESS);
  Wire.write(kPcaMode1);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom(PCA9685_ADDRESS, static_cast<uint8_t>(1)) != 1) {
    return false;
  }

  Wire.read();
  return true;
}

}  // namespace

void updateI2cRecovery(uint32_t nowMs) {
  if (nowMs - g_lastCheckMs < kCheckMs) {
    return;
  }

  g_lastCheckMs = nowMs;

  if (pcaReadOk()) {
    return;
  }

  g_resetCount++;
  Wire.end();
  Wire.begin(I2C_SDA, I2C_SCL);

  // Raw core log: visible on USB even when the serial_log setting is off.
  log_e(
    "I2C bus reset #%lu: PCA9685 read %s after reset",
    static_cast<unsigned long>(g_resetCount),
    pcaReadOk() ? "OK" : "still failing"
  );
}
