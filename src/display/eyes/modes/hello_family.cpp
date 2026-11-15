#include "display/eyes/modes/hello_family.h"

#include <Arduino.h>

#include "animation/hello_family.h"
#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

void startHelloFamilyEyes(uint32_t now) {
  (void)now;
}

// Wide, raised eyes; one blink between "Hello" and "family".
void updateHelloFamilyEyes(uint32_t now) {
  int16_t height = 16;
  int16_t yOffset = -2;

  if (helloFamilyLowering()) {
    height = 14;
    yOffset = 0;
  } else {
    const uint32_t audioElapsed = helloFamilyAudioElapsed(now);

    if (audioElapsed >= HELLO_FAMILY_BLINK_START_MS &&
        audioElapsed < HELLO_FAMILY_BLINK_END_MS) {
      const uint32_t blinkMid =
        (HELLO_FAMILY_BLINK_START_MS + HELLO_FAMILY_BLINK_END_MS) / 2;
      const uint32_t halfBlink =
        (HELLO_FAMILY_BLINK_END_MS - HELLO_FAMILY_BLINK_START_MS) / 2;
      const float blinkT = audioElapsed < blinkMid
        ? (float)(audioElapsed - HELLO_FAMILY_BLINK_START_MS) /
          (float)halfBlink
        : (float)(HELLO_FAMILY_BLINK_END_MS - audioElapsed) /
          (float)halfBlink;
      const float open = anim::easeInOutCubic(
        constrain(blinkT, 0.0f, 1.0f)
      );
      height = (int16_t)(2.0f + 14.0f * open);
    }
  }

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    eyes::DEFAULT_LEFT.x,
    eyes::DEFAULT_LEFT.width,
    height
  );
  right = eyes::eyeWithHeight(
    eyes::DEFAULT_RIGHT.x,
    eyes::DEFAULT_RIGHT.width,
    height
  );
  left.y += yOffset;
  right.y += yOffset;
}
