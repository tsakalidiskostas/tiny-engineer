#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"
#include "display/oled.h"
#include "display/oled_internal.h"
#include "serial_log.h"
#include "settings/settings.h"

bool i2cDeviceConnected(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void initOled() {
  serialLogPrintln();
  serialLogPrintln("Checking OLED at 0x3C...");

  if (!i2cDeviceConnected(OLED_ADDRESS)) {
    serialLogPrintln("ERROR: OLED not found");
    oledAvailable = false;
    return;
  }

  serialLogPrintln("OLED found");

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {
    serialLogPrintln("ERROR: OLED initialization failed");
    oledAvailable = false;
    return;
  }

  display.setRotation(settingsOledRotate180() ? 2 : 0);

  display.clearDisplay();
  display.display();

  oledAvailable = true;

  serialLogPrintln("OLED initialized");
}

namespace {

constexpr uint32_t kOledCheckMs = 1000;

uint32_t g_lastOledCheckMs = 0;
bool g_oledFoundAtBoot = false;
bool g_oledLost = false;

// A panel that browned out restarts with power-on defaults: page addressing
// and no remap, which shows as garbled or upside-down eyes while it still ACKs.
void refreshOledAddressing() {
  display.ssd1306_command(SSD1306_MEMORYMODE);
  display.ssd1306_command(0x00);
  display.ssd1306_command(SSD1306_SEGREMAP | 0x1);
  display.ssd1306_command(SSD1306_COMSCANDEC);
}

}  // namespace

// Every write to a panel that stopped answering stalls the loop (~60 ms),
// which makes audio choppy. Ping once a second, stop drawing while it is
// gone, and re-run init when it answers again.
void updateOledHealth(uint32_t nowMs) {
  if (!g_oledFoundAtBoot) {
    g_oledFoundAtBoot = oledAvailable;

    if (!g_oledFoundAtBoot) {
      return;
    }
  }

  if (nowMs - g_lastOledCheckMs < kOledCheckMs) {
    return;
  }

  g_lastOledCheckMs = nowMs;

  const bool answering = i2cDeviceConnected(OLED_ADDRESS);

  if (!g_oledLost) {
    if (answering) {
      refreshOledAddressing();
      return;
    }

    g_oledLost = true;
    oledAvailable = false;
    serialLogPrintln("OLED stopped answering; drawing paused");
    return;
  }

  if (!answering) {
    return;
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS, false, false)) {
    return;
  }

  display.setRotation(settingsOledRotate180() ? 2 : 0);
  g_oledLost = false;
  oledAvailable = true;
  serialLogPrintln("OLED answering again; re-initialized");
}
