# tiny-engineer

Unified CLI for the [Tiny Engineer](https://github.com/jamro/tiny-engineer) desk robot: install IDE hooks, map agent events to poses, and drive animations over HTTP or USB serial.

## Install / run

```bash
npx -y tiny-engineer setup
npx -y tiny-engineer setup cursor --yes
npx -y tiny-engineer doctor
npx -y tiny-engineer anim ring
```

Supported IDEs: **Cursor**, **Claude Code**, **Antigravity**.

## Config

Connection prefs (transport, URL, port, token) live in **user config** (`~/.config/tiny-engineer/env`; Windows `%APPDATA%\tiny-engineer\env`). Project `.env` overrides when present. A user/process `TINY_ENGINEER_TOKEN` is **not** sent as `Authorization: Bearer` when the URL came only from project `.env` (request continues without Bearer; stderr warns). Details: [integration guide](https://github.com/jamro/tiny-engineer/blob/main/docs/integration.md).

## Sound output

`tiny-engineer audio computer` plays the sound messages (welcome, bell, attention, error, abort, dead, hello_family, and `play` clips) on this computer's default output, e.g. headphones. The robot keeps animating with its volume at 0. `tiny-engineer audio robot` restores the saved volume and plays on the robot again. `tiny-engineer audio` prints the current choice, stored as `TINY_ENGINEER_AUDIO` in user config. Clips come from `TINY_ENGINEER_CLIPS`, else the repo's `data/` (with any `custom_audio_mod` overlay), else `assets/`. Players: PowerShell on Windows, `afplay` on macOS, `aplay` on Linux.

## Docs

- [Integration guide](https://github.com/jamro/tiny-engineer/blob/main/docs/integration.md) (`setup` / hooks)
- [HTTP API](https://github.com/jamro/tiny-engineer/blob/main/docs/api.md) (custom agents / advanced)
- [CLI release process](https://github.com/jamro/tiny-engineer/blob/main/docs/cli-release.md) (maintainers: npm publish, separate from firmware `v*` tags)

## License

MIT — see [LICENSE](./LICENSE).
