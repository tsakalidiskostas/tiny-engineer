import { spawn } from "node:child_process";
import { existsSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

export const AUDIO_KEY = "TINY_ENGINEER_AUDIO";
export const CLIPS_KEY = "TINY_ENGINEER_CLIPS";
/** Robot volume saved while the computer plays the clips, restored on switch back. */
export const ROBOT_VOLUME_KEY = "TINY_ENGINEER_ROBOT_VOLUME";
export const AUDIO_OUTPUTS = ["robot", "computer"];
export const DEFAULT_ROBOT_VOLUME = 70;

/** Animations that play a LittleFS clip on the robot (see docs/api.md). */
export const CLIP_FOR_ANIM = {
  welcome: "welcome.wav",
  ring: "bell.wav",
  attention: "attention.wav",
  error: "error.wav",
  abort: "abort.wav",
  dead: "dead.wav",
  hello_family: "hello_family.wav",
};

// Repo root when the CLI runs from a checkout: packages/tiny-engineer-cli/src/audio.
const REPO_ROOT = resolve(dirname(fileURLToPath(import.meta.url)), "..", "..", "..", "..");

/**
 * @param {string | undefined | null} value
 * @returns {"robot" | "computer" | null}
 */
export function normalizeAudioOutput(value) {
  if (value === undefined || value === null) return null;
  const trimmed = String(value).trim().toLowerCase();
  if (trimmed === "robot" || trimmed === "speaker") return "robot";
  if (trimmed === "computer" || trimmed === "headphones") return "computer";
  return null;
}

/**
 * @returns {"robot" | "computer"} TINY_ENGINEER_AUDIO, default robot
 */
export function getAudioOutput() {
  return normalizeAudioOutput(process.env[AUDIO_KEY]) ?? "robot";
}

/**
 * Folders searched for clips: TINY_ENGINEER_CLIPS, then the built `data/`
 * (includes the custom_audio_mod overlay), then stock `assets/`.
 * @returns {string[]}
 */
export function clipDirs() {
  const dirs = [];
  const override = process.env[CLIPS_KEY]?.trim();
  if (override) dirs.push(override);
  dirs.push(join(REPO_ROOT, "data"), join(REPO_ROOT, "assets"));
  return dirs;
}

/**
 * @param {string} anim
 * @param {string[]} [dirs]
 * @returns {string | null} path of the clip the robot would play for `anim`
 */
export function findClipForAnim(anim, dirs = clipDirs()) {
  const file = CLIP_FOR_ANIM[/** @type {keyof typeof CLIP_FOR_ANIM} */ (anim)];
  if (!file) return null;
  for (const dir of dirs) {
    const path = join(dir, file);
    if (existsSync(path)) return path;
  }
  return null;
}

/**
 * Command that plays a WAV on the computer's default output device.
 * @param {string} wavPath
 * @param {NodeJS.Platform} [platform]
 * @returns {{ command: string, args: string[] }}
 */
export function playerCommand(wavPath, platform = process.platform) {
  if (platform === "win32") {
    const quoted = `'${wavPath.replace(/'/g, "''")}'`;
    return {
      command: "powershell.exe",
      args: [
        "-NoProfile",
        "-NonInteractive",
        "-Command",
        `(New-Object Media.SoundPlayer ${quoted}).PlaySync()`,
      ],
    };
  }
  if (platform === "darwin") return { command: "afplay", args: [wavPath] };
  return { command: "aplay", args: ["-q", wavPath] };
}

/**
 * Play a WAV on the computer. Detached so a hook can exit while it plays.
 * Never throws: a missing player only loses the sound.
 * @param {string} wavPath
 * @param {{ wait?: boolean }} [opts] wait=true resolves when playback ends
 * @returns {Promise<boolean>} false when the player could not start
 */
export function playOnComputer(wavPath, { wait = false } = {}) {
  const { command, args } = playerCommand(wavPath);
  return new Promise((resolvePlay) => {
    let child;
    try {
      child = spawn(command, args, {
        detached: !wait,
        stdio: "ignore",
        windowsHide: true,
      });
    } catch {
      resolvePlay(false);
      return;
    }
    child.on("error", () => resolvePlay(false));
    if (wait) {
      child.on("close", (code) => resolvePlay(code === 0));
      return;
    }
    child.on("spawn", () => {
      child.unref();
      resolvePlay(true);
    });
  });
}

/**
 * When audio goes to the computer, play the clip that `anim` would play on
 * the robot. No-op for animations without a clip or in robot mode.
 * @param {string} anim
 * @returns {Promise<boolean>} true when a clip started on the computer
 */
export async function playAnimClipOnComputer(anim) {
  if (getAudioOutput() !== "computer") return false;
  const clip = findClipForAnim(anim);
  if (!clip) return false;
  return playOnComputer(clip);
}
