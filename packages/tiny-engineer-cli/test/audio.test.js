import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, readFileSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

import {
  AUDIO_KEY,
  findClipForAnim,
  getAudioOutput,
  normalizeAudioOutput,
  playerCommand,
} from "../src/audio/local.js";
import { runAudio } from "../src/commands/audio.js";
import { createOutput } from "../src/util/output.js";
import { createStyle } from "../src/util/style.js";

function quietOut() {
  /** @type {string[]} */
  const errs = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => errs.push(String(m)),
  });
  return { out, errs };
}

test("normalizeAudioOutput accepts speaker/headphones aliases", () => {
  assert.equal(normalizeAudioOutput("robot"), "robot");
  assert.equal(normalizeAudioOutput("Speaker"), "robot");
  assert.equal(normalizeAudioOutput("computer"), "computer");
  assert.equal(normalizeAudioOutput(" headphones "), "computer");
  assert.equal(normalizeAudioOutput("both"), null);
  assert.equal(normalizeAudioOutput(undefined), null);
});

test("getAudioOutput defaults to robot", () => {
  const saved = process.env[AUDIO_KEY];
  try {
    delete process.env[AUDIO_KEY];
    assert.equal(getAudioOutput(), "robot");
    process.env[AUDIO_KEY] = "nonsense";
    assert.equal(getAudioOutput(), "robot");
    process.env[AUDIO_KEY] = "computer";
    assert.equal(getAudioOutput(), "computer");
  } finally {
    if (saved === undefined) delete process.env[AUDIO_KEY];
    else process.env[AUDIO_KEY] = saved;
  }
});

test("findClipForAnim maps animations to clips, first folder wins", () => {
  const modDir = mkdtempSync(join(tmpdir(), "te-clips-mod-"));
  const stockDir = mkdtempSync(join(tmpdir(), "te-clips-stock-"));
  writeFileSync(join(stockDir, "bell.wav"), "");
  writeFileSync(join(stockDir, "error.wav"), "");
  writeFileSync(join(modDir, "error.wav"), "");

  assert.equal(findClipForAnim("ring", [modDir, stockDir]), join(stockDir, "bell.wav"));
  assert.equal(findClipForAnim("error", [modDir, stockDir]), join(modDir, "error.wav"));
  assert.equal(findClipForAnim("attention", [modDir, stockDir]), null);
  assert.equal(findClipForAnim("typing", [modDir, stockDir]), null);
});

test("playerCommand uses the platform's player and escapes PowerShell quotes", () => {
  const win = playerCommand("C:\\Users\\O'Brien\\bell.wav", "win32");
  assert.equal(win.command, "powershell.exe");
  assert.ok(win.args.at(-1)?.includes("'C:\\Users\\O''Brien\\bell.wav'"));
  assert.deepEqual(playerCommand("/x/bell.wav", "darwin"), { command: "afplay", args: ["/x/bell.wav"] });
  assert.deepEqual(playerCommand("/x/bell.wav", "linux"), { command: "aplay", args: ["-q", "/x/bell.wav"] });
});

test("runAudio rejects an unknown output without touching config", async () => {
  const envPath = join(mkdtempSync(join(tmpdir(), "te-audio-")), "env");
  const { out, errs } = quietOut();
  await runAudio(out, ["both"], { envPath });
  assert.equal(process.exitCode, 1);
  assert.ok(errs.some((e) => e.includes("robot or computer")));
  assert.throws(() => readFileSync(envPath, "utf8"));
  process.exitCode = 0;
});

test("runAudio saves the choice even when the robot is unreachable", async () => {
  const envPath = join(mkdtempSync(join(tmpdir(), "te-audio-")), "env");
  const saved = process.env[AUDIO_KEY];
  const { out, errs } = quietOut();
  try {
    delete process.env[AUDIO_KEY];
    await runAudio(out, ["headphones", "--url", "http://127.0.0.1:9"], { envPath });
    assert.equal(process.exitCode, 1);
    assert.ok(errs.some((e) => e.includes("could not mute")));
    assert.match(readFileSync(envPath, "utf8"), /^TINY_ENGINEER_AUDIO=computer$/m);
  } finally {
    if (saved === undefined) delete process.env[AUDIO_KEY];
    else process.env[AUDIO_KEY] = saved;
    process.exitCode = 0;
  }
});
