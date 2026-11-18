import {
  AUDIO_KEY,
  DEFAULT_ROBOT_VOLUME,
  ROBOT_VOLUME_KEY,
  getAudioOutput,
  normalizeAudioOutput,
} from "../audio/local.js";
import { userEnvPath } from "../config/paths.js";
import { robotSettingsGet, robotSettingsSet } from "../robot/ops.js";
import { formatRobotError } from "../robot/transport.js";
import { parseCommonArgs } from "../util/args.js";
import { mergeEnvKey, readEnvKey } from "../wizard/merge.js";

const USAGE = `Usage: tiny-engineer audio [robot|computer] [options]

Choose where sound messages (welcome, bell, attention, error, abort, …) play.

  audio             Show the current output
  audio robot       Play on the robot's speaker (alias: speaker)
  audio computer    Play on this computer's default output, e.g. headphones
                    (alias: headphones). The robot keeps animating, muted.

Switching to computer saves the robot's volume and sets it to 0; switching
back restores it. The choice is saved as ${AUDIO_KEY} in the user config.
Clips come from TINY_ENGINEER_CLIPS, else the repo's data/, else assets/.

Options:
  --transport <http|serial>  --url <base>  --port <path>
`;

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {{ envPath?: string }} [deps]
 */
export async function runAudio(out, argv, { envPath = userEnvPath() } = {}) {
  const opts = parseCommonArgs(argv);
  if (opts.error) {
    out.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    console.log(USAGE);
    return;
  }

  const [choice, extra] = opts.rest;
  if (extra !== undefined) {
    out.error(`Unknown argument: ${extra}`);
    process.exitCode = 1;
    return;
  }
  if (choice === undefined) {
    console.log(getAudioOutput());
    process.exitCode = 0;
    return;
  }

  const target = normalizeAudioOutput(choice);
  if (!target) {
    out.error(`audio needs robot or computer (got ${choice})`);
    process.exitCode = 1;
    return;
  }

  const transportOpts = { url: opts.url, transport: opts.transport, port: opts.port };
  const current = getAudioOutput();
  let muteError = null;

  try {
    if (target === "computer" && current !== "computer") {
      const got = await robotSettingsGet(transportOpts);
      const volume = got.ok ? Number(got.bodyJson?.volume) : NaN;
      if (Number.isInteger(volume) && volume > 0) {
        mergeEnvKey(envPath, ROBOT_VOLUME_KEY, String(volume), { force: true });
      }
      const set = got.ok ? await robotSettingsSet({ volume: 0 }, transportOpts) : got;
      if (!set.ok) muteError = formatRobotError(set);
    } else if (target === "robot" && current !== "robot") {
      const saved = Number(readEnvKey(envPath, ROBOT_VOLUME_KEY));
      const volume = Number.isInteger(saved) && saved > 0 ? saved : DEFAULT_ROBOT_VOLUME;
      const set = await robotSettingsSet({ volume }, transportOpts);
      if (!set.ok) muteError = formatRobotError(set);
    }
  } catch (err) {
    muteError = `robot unreachable: ${err?.cause?.message ?? err?.message ?? String(err)}`;
  }

  mergeEnvKey(envPath, AUDIO_KEY, target, { force: true });
  process.env[AUDIO_KEY] = target;

  if (muteError) {
    const action = target === "computer" ? "mute" : "unmute";
    out.error(`audio set to ${target}, but could not ${action} the robot: ${muteError}`);
    process.exitCode = 1;
    return;
  }
  out.ok(
    target === "computer"
      ? "sound messages → this computer (robot speaker muted)"
      : "sound messages → robot speaker",
  );
  process.exitCode = 0;
}
