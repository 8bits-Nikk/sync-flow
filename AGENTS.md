# AGENTS.md — working ON the Sync Flow repo

This file is for AI coding agents (and people) changing this repository. Agents *using* the published
package read `packages/sync-flow/AGENTS.md` instead.

## Read first, in this order

1. [PLAN.md](PLAN.md) §4–9: the API, wire protocol, storage and rules. They are the contract.
2. [DECISIONS.md](DECISIONS.md): every settled decision, with a **Rule** your code must follow.
3. [MILESTONES.md](MILESTONES.md): find your task ID, its **Done when**, and the milestone gate.

Never reverse a decision in code. If a task seems to need that, stop and report it; a decision only
changes through a new DECISIONS.md entry approved by the owner.

## Layout

| Path | What |
|---|---|
| `packages/sync-flow/` | App package: C++ core (`cpp/`), Swift (`ios/`), Kotlin (`android/`), Nitro bindings (`nitrogen/`), TypeScript (`src/`) |
| `packages/sync-flow/cpp/third_party/` | Vendored SQLite3MultipleCiphers and yyjson. Never edit; update by replacing the whole folder and the version in its `VERSION` file |
| `packages/sync-flow-server/` | Server kit, CLI and standalone server (TypeScript) |
| `spec/` | Normative wire protocol (`protocol.md`) and `openapi.yaml` |
| `examples/` | Example and pilot apps |

## Commands

```bash
corepack enable            # once; uses the pnpm version pinned in package.json
pnpm i
pnpm build                 # TypeScript builds (tsup)
pnpm typecheck
pnpm lint                  # eslint + prettier --check (run `pnpm format` to fix)
pnpm test                  # vitest in every package
pnpm test:cpp              # C++ core + GoogleTest on the host (needs cmake + ninja)
```

All of these must pass before you commit. CI runs the same commands plus native builds.

## Rules for changes

- **One task per commit or PR.** Title format: `<task ID> <summary> (<decision IDs>)`, e.g.
  `M1-07 outbox coalescing (D-21)`. Tick the task in MILESTONES.md and update its progress table in
  the same commit.
- **TypeScript:** strict mode, no `any`, no default exports. Public API changes must match PLAN.md §4
  exactly (D-25); if they don't, the plan is updated first.
- **C++:** C++20, no exceptions across the JSI boundary (convert to `SyncFlowError` codes), RAII for
  every SQLite handle. Platform-specific code stays in `ios/` and `android/`; `cpp/` must build on
  Linux too (D-04, D-32, D-34).
- **Errors:** every new error code gets a `fix` and a docs entry (D-27).
- **Dependencies:** don't add a runtime dependency to either package without a DECISIONS.md entry
  (D-02). Dev dependencies are fine; pin exact versions.
- **Secrets and data:** never log row data (D-18 logs rule), never commit keys or `.env` files.

## Toolchain versions

Node 22 (`.nvmrc`), pnpm 10, TypeScript 6.0 (the JS-based compiler; TS 7 waits until tsup and
typescript-eslint support it), CMake ≥ 3.22, C++20.
