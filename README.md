# Sync Flow

Offline-first data for React Native and Expo, with the sync logic done for you.

- **Instant UI:** reads and writes hit an encrypted local SQLite database; sync runs in native C++ threads.
- **Any backend:** two JSON endpoints. Use the Node kit, the standalone server (Docker), or implement the spec.
- **Correct by default:** idempotent writes, ordered outbox, field-level merge, safe change cursors, tombstones.
- **Free:** MIT, published to npm and GitHub only.

> **Status: under construction (milestone M0).** Nothing is usable yet. Follow progress in [MILESTONES.md](MILESTONES.md).

| Doc | What |
|---|---|
| [PLAN.md](PLAN.md) | Design: problem, API, protocol, engine, roadmap |
| [DECISIONS.md](DECISIONS.md) | Every decision with its reason |
| [MILESTONES.md](MILESTONES.md) | Task checklist and progress |
| [AGENTS.md](AGENTS.md) | How to work on this repo |

## Packages

| Package | |
|---|---|
| `@nikhilll/sync-flow` | The app package (React Native / Expo dev builds, New Architecture) |
| `@nikhilll/sync-flow-server` | Node server kit, CLI and standalone server |

## Licence

[MIT](LICENSE)
