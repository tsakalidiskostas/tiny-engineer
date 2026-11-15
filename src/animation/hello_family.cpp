#include <Arduino.h>
#include <math.h>

#include "animation.h"
#include "animation/constants.h"
#include "animation/hello_family.h"
#include "animation/util.h"
#include "audio/audio.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"

using anim::easedLerp;
using anim::stopAnimServos;

namespace {

constexpr uint32_t RAISE_MS = 430;
constexpr uint32_t LOWER_MS = 600;
constexpr uint32_t HELLO_FAMILY_SILENT_MS = 2930;

constexpr float HAND_RIGHT_REST = -1.0f;
constexpr float HAND_RIGHT_RAISED = 1.0f;
constexpr float HAND_LEFT_REST = 1.0f;
constexpr float HAND_LEFT_RAISED = -1.0f;
constexpr float HAND_WIGGLE = 8.0f / 45.0f;
constexpr float NECK_SIDE = 2.0f / 9.0f;

uint32_t g_startMs = 0;
uint32_t g_audioStartMs = 0;
uint32_t g_lowerStartMs = 0;
bool g_audioStarted = false;
bool g_silent = false;
bool g_lowering = false;

void applyRaisePose(uint32_t now) {
  servoAt(SERVO_HAND_RIGHT).setNormPosition(
    easedLerp(HAND_RIGHT_REST, HAND_RIGHT_RAISED, g_startMs, RAISE_MS, now)
  );
  servoAt(SERVO_HAND_LEFT).setNormPosition(
    easedLerp(HAND_LEFT_REST, HAND_LEFT_RAISED, g_startMs, RAISE_MS, now)
  );
  servoAt(SERVO_HEAD).setNormPosition(
    easedLerp(anim::WELCOME_HEAD_MID, anim::WELCOME_HEAD_UP, g_startMs, RAISE_MS, now)
  );
  servoAt(SERVO_NECK).setNormPosition(
    easedLerp(0.0f, -NECK_SIDE, g_startMs, RAISE_MS, now)
  );
}

// "Hello" looks to one side; "family" sweeps the head across to the other.
void applyGreetingPose(uint32_t audioElapsed, uint32_t now) {
  const float t = (float)audioElapsed / 1000.0f;
  // Two waves per second; the hands dip in turn.
  const float wave = sinf(t * 2.0f * PI * 2.0f) * HAND_WIGGLE;

  servoAt(SERVO_HAND_RIGHT).setNormPosition(HAND_RIGHT_RAISED - fmaxf(wave, 0.0f));
  servoAt(SERVO_HAND_LEFT).setNormPosition(HAND_LEFT_RAISED + fmaxf(-wave, 0.0f));
  servoAt(SERVO_HEAD).setNormPosition(anim::WELCOME_HEAD_UP);

  float neck = -NECK_SIDE;

  if (audioElapsed >= HELLO_FAMILY_HELLO_END_MS) {
    neck = easedLerp(
      -NECK_SIDE,
      NECK_SIDE,
      g_audioStartMs + HELLO_FAMILY_HELLO_END_MS,
      HELLO_FAMILY_SPEECH_END_MS - HELLO_FAMILY_HELLO_END_MS,
      now
    );
  }

  servoAt(SERVO_NECK).setNormPosition(neck);
}

void applyLowerPose(uint32_t now) {
  servoAt(SERVO_HAND_RIGHT).setNormPosition(
    easedLerp(HAND_RIGHT_RAISED, HAND_RIGHT_REST, g_lowerStartMs, LOWER_MS, now)
  );
  servoAt(SERVO_HAND_LEFT).setNormPosition(
    easedLerp(HAND_LEFT_RAISED, HAND_LEFT_REST, g_lowerStartMs, LOWER_MS, now)
  );
  servoAt(SERVO_HEAD).setNormPosition(
    easedLerp(anim::WELCOME_HEAD_UP, anim::WELCOME_HEAD_MID, g_lowerStartMs, LOWER_MS, now)
  );
  servoAt(SERVO_NECK).setNormPosition(
    easedLerp(NECK_SIDE, 0.0f, g_lowerStartMs, LOWER_MS, now)
  );
}

void startLowering(uint32_t now) {
  g_lowering = true;
  g_lowerStartMs = now;
}

}  // namespace

bool helloFamilyLowering() {
  return g_lowering;
}

uint32_t helloFamilyAudioElapsed(uint32_t now) {
  if (!g_audioStarted) {
    return 0;
  }

  return now - g_audioStartMs;
}

void startHelloFamily() {
  stopAllWavPlayback();
  stopAnimServos();
  anim::parkForTransition();

  g_startMs = millis();
  g_audioStartMs = 0;
  g_lowerStartMs = 0;
  g_audioStarted = false;
  g_silent = false;
  g_lowering = false;
}

void updateHelloFamily(uint32_t now) {
  servoAt(SERVO_BODY).update();

  if (!g_audioStarted) {
    applyRaisePose(now);

    if (now - g_startMs >= RAISE_MS) {
      g_audioStarted = true;
      g_audioStartMs = now;
      g_silent = !startHelloFamilyPlayback();
    }
    return;
  }

  if (g_lowering) {
    applyLowerPose(now);

    if (now - g_lowerStartMs >= LOWER_MS) {
      finishAnimation(now);
    }
    return;
  }

  const uint32_t audioElapsed = helloFamilyAudioElapsed(now);
  const bool playing = g_silent
    ? audioElapsed < HELLO_FAMILY_SILENT_MS
    : updateHelloFamilyPlayback();

  applyGreetingPose(audioElapsed, now);

  if (!playing) {
    startLowering(now);
  }
}
