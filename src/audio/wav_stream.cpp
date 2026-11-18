#include "audio/wav_stream.h"

#include <Arduino.h>
#include <ESP_I2S.h>
#include <FS.h>
#include <LittleFS.h>

#include "audio/audio.h"
#include "audio/wav_header.h"
#include "pins.h"
#include "settings/settings.h"
#include "serial_log.h"

namespace {

constexpr int kAudioFrames = 512;

struct WavClipDef {
  const char* path;
  const char* label;
  const char* oledTitle;
};

constexpr WavClipDef kClips[kWavClipCount] = {
  {"/bell.wav", "Bell", "BELL"},
  {"/welcome.wav", "Welcome", "WELCOME"},
  {"/attention.wav", "Attention", "ATTENTION"},
  {"/error.wav", "Error", "ERROR"},
  {"/abort.wav", "Abort", "ABORT"},
  {"/dead.wav", "Dead", "DEAD"},
  {"/hello_family.wav", "Hello family", "HELLO FAMILY"},
};

// Lowest clip rate accepted; clips below SAMPLE_RATE are upsampled on the fly.
constexpr uint32_t kMinClipRate = 8000;

File g_file;
bool g_playing = false;
WavClip g_active = WavClip::Bell;

// Linear-interpolation upsampler state. Phase is the position between
// prev and next in 1/65536ths of a source sample.
struct Upsampler {
  int16_t in[kAudioFrames];
  size_t inLen;
  size_t inPos;
  int16_t prev;
  int16_t next;
  uint32_t phase;
  uint32_t step;
  bool primed;
};

bool g_upsample = false;
Upsampler g_up;

void logAudioStorageContents() {
  File root = LittleFS.open("/");

  if (!root || !root.isDirectory()) {
    serialLogPrintln("LittleFS root unavailable");
    return;
  }

  serialLogPrintln("LittleFS files:");

  File entry = root.openNextFile();

  while (entry) {
    serialLogPrint("  ");
    serialLogPrintln(entry.name());
    entry.close();
    entry = root.openNextFile();
  }

  root.close();
}

struct WavFileReader {
  File& file;

  bool read(uint8_t* dest, size_t len) {
    return file.read(dest, len) == len;
  }

  bool skip(uint32_t len) {
    return file.seek(file.position() + len);
  }
};

bool pumpWavChunk(File& file) {
  int16_t samples[kAudioFrames];

  const size_t bytesRead =
    file.read(
      reinterpret_cast<uint8_t*>(samples),
      sizeof(samples)
    );

  if (bytesRead < 2) {
    return false;
  }

  writeMonoToSpeaker(samples, bytesRead / 2);

  return file.available() > 0;
}

bool nextSourceSample(File& file, int16_t& sample) {
  if (g_up.inPos >= g_up.inLen) {
    const size_t bytesRead =
      file.read(reinterpret_cast<uint8_t*>(g_up.in), sizeof(g_up.in));
    g_up.inLen = bytesRead / 2;
    g_up.inPos = 0;

    if (g_up.inLen == 0) {
      return false;
    }
  }

  sample = g_up.in[g_up.inPos++];
  return true;
}

bool pumpUpsampledChunk(File& file) {
  if (!g_up.primed) {
    if (!nextSourceSample(file, g_up.prev)) {
      return false;
    }
    if (!nextSourceSample(file, g_up.next)) {
      g_up.next = g_up.prev;
    }
    g_up.primed = true;
  }

  int16_t out[kAudioFrames];
  size_t count = 0;
  bool more = true;

  while (count < kAudioFrames) {
    const int32_t delta = static_cast<int32_t>(g_up.next) - g_up.prev;
    out[count++] = static_cast<int16_t>(
      g_up.prev + static_cast<int32_t>((static_cast<int64_t>(delta) * g_up.phase) >> 16)
    );

    g_up.phase += g_up.step;

    while (g_up.phase >= 0x10000) {
      g_up.phase -= 0x10000;
      g_up.prev = g_up.next;

      if (!nextSourceSample(file, g_up.next)) {
        more = false;
        break;
      }
    }

    if (!more) {
      break;
    }
  }

  writeMonoToSpeaker(out, count);
  return more;
}

const WavClipDef& clipDef(WavClip clip) {
  return kClips[static_cast<uint8_t>(clip)];
}

}  // namespace

const char* wavClipPath(WavClip clip) {
  return clipDef(clip).path;
}

const char* wavClipOledTitle(WavClip clip) {
  return clipDef(clip).oledTitle;
}

bool wavStreamStart(WavClip clip) {
  wavStreamStop();

  const WavClipDef& def = clipDef(clip);

  if (!initAudioStorage()) {
    serialLogPrint(def.label);
    serialLogPrintln(" failed: filesystem");
    return false;
  }

  g_file = LittleFS.open(def.path, "r");

  if (!g_file) {
    logAudioStorageContents();
    serialLogPrint(def.label);
    serialLogPrint(" failed: ");
    serialLogPrint(def.path);
    serialLogPrintln(" missing");
    return false;
  }

  WavFileReader reader{g_file};
  uint32_t rate = 0;
  WavHeaderResult header = readWavHeader(reader, 0, &rate);

  if (header == WavHeaderResult::Ok &&
      (rate < kMinClipRate || rate > static_cast<uint32_t>(SAMPLE_RATE))) {
    header = WavHeaderResult::SampleRateMismatch;
  }

  if (header != WavHeaderResult::Ok) {
    g_file.close();
    serialLogPrint(def.label);
    serialLogPrint(" failed: invalid WAV (");
    serialLogPrint(wavHeaderResultText(header));
    serialLogPrintln(")");
    return false;
  }

  g_upsample = rate != static_cast<uint32_t>(SAMPLE_RATE);
  if (g_upsample) {
    g_up = {};
    g_up.step = static_cast<uint32_t>(
      (static_cast<uint64_t>(rate) << 16) / static_cast<uint32_t>(SAMPLE_RATE)
    );
  }

  g_active = clip;
  g_playing = true;
  serialLogPrint("Playing ");
  serialLogPrintln(def.path);
  return true;
}

bool wavStreamUpdate() {
  if (!g_playing) {
    return false;
  }

  const WavClipDef& def = clipDef(g_active);

  const bool more =
    g_upsample ? pumpUpsampledChunk(g_file) : pumpWavChunk(g_file);

  if (!more) {
    if (g_file) {
      g_file.close();
    }
    g_playing = false;
    serialLogPrint(def.label);
    serialLogPrintln(" OK");
    return false;
  }

  return true;
}

void wavStreamStop() {
  if (g_file) {
    g_file.close();
  }

  g_playing = false;
}

bool wavStreamIsPlaying(WavClip clip) {
  return g_playing && g_active == clip;
}
