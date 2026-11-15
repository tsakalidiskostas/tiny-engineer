#pragma once

#include <stdint.h>

// Mod-only hello_family.wav: Greek greeting / short pause / "family" (~2.9 s).
// Without the WAV the gesture runs silent for the same length.
constexpr uint32_t HELLO_FAMILY_HELLO_END_MS = 560;
constexpr uint32_t HELLO_FAMILY_BLINK_START_MS = 560;
constexpr uint32_t HELLO_FAMILY_BLINK_END_MS = 620;
constexpr uint32_t HELLO_FAMILY_SPEECH_END_MS = 2570;

void startHelloFamily();
void updateHelloFamily(uint32_t now);
bool helloFamilyLowering();
uint32_t helloFamilyAudioElapsed(uint32_t now);
