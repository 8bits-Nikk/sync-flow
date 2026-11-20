# Sync Flow — Milestones and tasks

The build broken into milestones and tasks, so we can see what is done and what is next.
The *what* and *why* live in [PLAN.md](PLAN.md) and [DECISIONS.md](DECISIONS.md); this file is the *checklist*.

## How to use this file

- **Who does it:**
  - 👤 **You** do the task: accounts, approvals, devices, decisions. The **How** line tells you exactly what to do.
  - 🤖 **Agent** does the task (Claude or another coding agent). You review the PR.
  - 👥 **Both** means the agent prepares it and you check or approve.
- **Status:** `[ ]` todo · `[~]` in progress · `[x]` done · `[-]` dropped (say why in the task).
- **Done when** is the test of finished. A task isn't done until that is true *and* CI is green.
- **Ordering:** work top to bottom inside a milestone. A milestone's **gate** must pass before the next one starts.
- **Commits and PRs:** one task per PR when possible. Put the task ID in the title, plus any decision IDs it touches, e.g. `M1-07 outbox coalescing (D-21)`.
- **When a task finishes:** tick it here and update the progress table, in the same PR.

## Progress

| Milestone | Weeks | Tasks | Done | Gate | Status |
|---|---|---|---:|---|---|
| [M0 Skeleton](#m0-skeleton-week-1) | 1 | 14 | 12 | Empty Expo app opens an encrypted DB from JS | Gate passed in CI; owner review pending |
| [M1 Local core](#m1-local-core-weeks-24) | 2–4 | 14 | 0 | **A:** local-only todo app works offline; local perf met | — |
| [M2 Sync](#m2-sync-weeks-57) | 5–7 | 17 | 0 | **B:** property tests converge; kill-mid-sync e2e; conformance green | — |
| [M3 Agent-proof 0.1.0](#m3-agent-proof-010-weeks-89) | 8–9 | 13 | 0 | **C:** a fresh agent adds sync to a blank Expo app unaided | — |
| [M3b Standalone server 0.2.0](#m3b-standalone-server--pilot-020-weeks-1011) | 10–11 | 15 | 0 | **D:** pilot syncs with zero sync code in the backend | — |
| [M4 0.5](#m4-05-weeks-1216) | 12–16 | 13 | 0 | Pilot in daily use 2 weeks, no data-loss bugs | — |
| [M5 1.0+](#m5-10-and-later) | — | 6 | 0 | — | — |
| **Total** | | **92** | **12** | | |

**Your tasks at a glance** (👤 and 👥): M0-01, M0-02, M0-03, M0-13, M0-14, M1-14, M2-17, M3-12, M3-13, M3b-14, M3b-15, M4-12, M4-13, M5-01, M5-06.
The rest is agent work you review.

---

## M0 Skeleton (week 1)

**Goal:** the repo exists, CI builds a Nitro module on iOS and Android, and SQLite3MultipleCiphers opens an encrypted database.
**Gate:** an empty Expo app (`examples/expo-todo`, prebuilt) calls `SyncFlow.open()` from JS, writes one row to an encrypted DB and reads it back, on an Android emulator and an iOS simulator, in CI.

- [x] **M0-01 👤 Create the GitHub repo**
  - **What:** public repo `8bits-Nikk/sync-flow`, MIT, empty.
  - **How:** github.com → New repository → name `sync-flow`, Public, no README (the agent pushes the first commit). Or reply "create repo" and I run `gh repo create 8bits-Nikk/sync-flow --public`.
  - **Done when:** the repo URL opens.
- [x] **M0-02 👤 Confirm the npm scope**
  - **What:** `@nikhilll/*` packages need the npm user `nikhilll` (or an org with that name) to be yours.
  - **How:** sign in at npmjs.com and check that the profile is `nikhilll`. Turn on 2FA if it's off (Account → Two-Factor Authentication).
  - **Done when:** you've confirmed it's your account.
- [x] **M0-03 👤 Decide where iOS gets built**
  - **What:** iOS builds need macOS. This server is Linux.
  - **How:** pick one: (a) GitHub Actions macOS runners only (free for public repos, slower feedback), or (b) also your own Mac for local debugging. Tell me which.
  - **Done when:** the choice is written here.
  - **Chosen (2026-11-20):** both. CI uses GitHub macOS runners, and the owner has a Mac for local debugging.
- [x] **M0-04 🤖 Monorepo scaffold**
  - **What:** pnpm workspaces, `packages/sync-flow`, `packages/sync-flow-server`, `examples/`, `spec/`, `docs/`, `bench/`, `skills/`. MIT `LICENSE`, root `AGENTS.md`, copy PLAN.md, DECISIONS.md and MILESTONES.md in. TypeScript strict, ESLint, Prettier, changesets (D-30).
  - **Done when:** `pnpm i && pnpm -r build && pnpm -r lint` passes on a clean clone.
- [x] **M0-05 🤖 Root `AGENTS.md` for contributors**
  - **What:** how to build, test and run each package; "read PLAN §4–9 and DECISIONS before coding"; commit format; never change a decision in code.
  - **Done when:** a new agent session can build the repo using only this file.
- [x] **M0-06 🤖 Nitro module skeleton**
  - **What:** `packages/sync-flow` as a Nitro module (`nitro.json`, Nitrogen output committed, D-04), with one HybridObject `SyncFlowCore` exposing `version(): string`.
  - **Done when:** the example app logs the version on both platforms.
- [x] **M0-07 🤖 Vendor SQLite3MultipleCiphers + yyjson**
  - **What:** add the amalgamations under `cpp/third_party/` with pinned versions and licence files (D-06, D-09). CMake for Android, podspec for iOS.
  - **Done when:** both compile in the module on Android NDK r27+ and current Xcode (this also checks the "to verify" item in PLAN §13).
- [x] **M0-08 🤖 Encrypted open/close**
  - **What:** C++ `Database::open(path, key)` with WAL, `synchronous=NORMAL` and `foreign_keys=OFF`. Key generation plus Keychain/Keystore storage in Swift/Kotlin (D-05, D-07).
  - **Done when:** an unencrypted `sqlite3` CLI fails to read the file and the app reads it fine.
- [x] **M0-09 🤖 C++ test harness**
  - **What:** GoogleTest under `cpp/tests`, built with CMake on Linux.
  - **Done when:** `pnpm test:cpp` runs one open/write/read test.
- [x] **M0-10 🤖 Expo example + config plugin stub**
  - **What:** `examples/expo-todo` (Expo SDK 55+, latest available) using the package; `app.plugin.js` auto-registered. In Expo Go, throw `SF_EXPO_GO` (D-03).
  - **Done when:** `npx expo prebuild && npx expo run:android` shows the row read back.
- [x] **M0-11 🤖 CI**
  - **What:** GitHub Actions: lint, typecheck, unit tests, C++ tests (Linux), Android emulator build + Maestro smoke, iOS simulator build + Maestro smoke (macOS runner).
  - **Done when:** all jobs are green on `main`.
- [x] **M0-12 🤖 `spec/protocol.md` v1**
  - **What:** PLAN §5 expanded into a normative spec (MUST/SHOULD), plus a first `spec/openapi.yaml`.
  - **Done when:** every request, response, status code and header in §5 is in the spec.
- [ ] **M0-13 👥 Reserve the npm names**
  - **What:** publish `0.0.0` placeholders of both packages so the names are held and trusted publishing can be set up (D-31).
  - **How:** the agent prepares the tarballs. You run `npm login`, then `npm publish --access public` in each package folder. Then on npmjs.com → each package → Settings → Trusted Publisher → GitHub Actions, repo `8bits-Nikk/sync-flow`, workflow `release.yml`.
  - **Done when:** both packages show on npmjs.com with a trusted publisher.
- [ ] **M0-14 👤 Gate review**
  - **How:** open the CI run of the gate test (agent sends the link), check both platforms are green, and reply "M0 done".

## M1 Local core (weeks 2–4)

**Goal:** everything works locally with no server: schema, CRUD, queries, live updates, per-user files.
**Gate A:** `examples/expo-todo` works fully offline (create, edit, delete, list, survives kill/relaunch), and the local perf targets in PLAN §10 are met.

- [ ] **M1-01 🤖 Schema builder** — `defineSchema`, `table`, `t.id/text/int/real/bool/date/json/ref`, `.optional()`, `.default()` in `src/schema/`, pure TS (D-25). **Done when:** type tests prove inference (a wrong column name is a compile error), and it imports in Node with no RN.
- [ ] **M1-02 🤖 Schema → SQLite DDL + local migrations** — create tables with hidden `_v` and `_sf_state` columns; additive changes migrate automatically; a `version` bump resets (D-15). **Done when:** C++ tests cover add table, add column, add index and a version bump.
- [ ] **M1-03 🤖 UUIDv7 in C++** (D-10). **Done when:** ids are unique across 1M generations and time-ordered.
- [ ] **M1-04 🤖 Writer thread + reader pool** (D-20). **Done when:** a stress test of 10k concurrent writes and reads has no lock errors.
- [ ] **M1-05 🤖 CRUD API** — `insert` (returns id), `update` (patch), `delete`, `get`, `transaction`. Promises resolve from the writer thread. **Done when:** unit and e2e tests pass.
- [ ] **M1-06 🤖 Outbox write path** — each write updates the row and `_sf_outbox` in one transaction (D-21). **Done when:** killing the process mid-write never leaves a row without its outbox entry (crash test).
- [ ] **M1-07 🤖 Outbox coalescing** — the four rules in D-21, `queued` only. **Done when:** a C++ test exists for every rule, plus one showing `inflight` is untouched.
- [ ] **M1-08 🤖 Query builder** — `where/orderBy/limit/offset`, operators `gt, gte, lt, lte, ne, in, like, $or`, fully typed. **Done when:** type tests and SQL output tests pass.
- [ ] **M1-09 🤖 Change tracking** — `sqlite3_update_hook` → one event per commit; raw SQL tables via `sqlite3_set_authorizer` (D-26). **Done when:** only queries on changed tables re-run (test counts the re-runs).
- [ ] **M1-10 🤖 React hooks** — `SyncFlowProvider` (opens and closes by `userId`), `useQuery` (synchronous first read, hash compare), `useRow`; `SF_SLOW_SYNC_READ` warning in dev. **Done when:** a render test shows no loading flash and no re-render when the result is unchanged.
- [ ] **M1-11 🤖 Raw SQL read-only** — `db.sql` works in `useQuery`; writes throw `SF_RAW_WRITE`. **Done when:** tested.
- [ ] **M1-12 🤖 Per-user files + `signOut`** — file naming and key per user; `SF_PENDING_WRITES`; `force` wipes the file, WAL and key (D-07). **Done when:** e2e: user A signs out, user B signs in and sees nothing of A's.
- [ ] **M1-13 🤖 Local benchmarks** — `bench/` measures insert p95 and the 50-row query p95. **Done when:** a report lands in `bench/results/` and CI warns on a >10% regression.
- [ ] **M1-14 👤 Gate A review**
  - **How:** install the example on your phone (the agent sends an APK, or you run `npx expo run:android`). Turn on airplane mode, add and edit 10 todos, kill the app, reopen it, and check they're all there. Sign out and in as another user and check the list is empty. Reply "Gate A done" or say what broke.

## M2 Sync (weeks 5–7)

**Goal:** real sync against a Node + Postgres server, correct under crashes and bad networks.
**Gate B:** property tests converge (≥ 10,000 random runs), the kill-mid-sync device e2e passes, and conformance is green against `examples/server-express-postgres`.

**Client**

- [ ] **M2-01 🤖 Native HTTP** — URLSession/OkHttp, gzip, bytes handed to C++ (D-05). **Done when:** a 20 MB response goes from the network to C++ without passing through JS (checked by a test hook).
- [ ] **M2-02 🤖 Pull** — paging until `hasMore=false`, yyjson parse, 500-row batches, cursor saved in the same transaction, deletes (D-09, D-23). **Done when:** a pull killed at a random point resumes with no duplicates or gaps.
- [ ] **M2-03 🤖 Push** — up to 100 mutations in outbox order; handle `applied`/`duplicate`/`rejected`/`conflict`; roll back on reject (D-14). **Done when:** tests for each status.
- [ ] **M2-04 🤖 Rebase + write checkpoint** (D-23). **Done when:** a test shows a pending edit never flickers back during a pull.
- [ ] **M2-05 🤖 Sync loop + triggers** — start/foreground, network regained, debounced write, 30 s interval, `db.sync()`. **Done when:** each trigger is tested.
- [ ] **M2-06 🤖 Retry, backoff, quarantine** (D-22). **Done when:** a failed row blocks only itself; `db.outbox.retry/discard` work.
- [ ] **M2-07 🤖 Auth handling** — Nitro `getToken` callback, cache, 401 → refresh once → `unauthorized` (D-24). **Done when:** an expired-token test keeps the outbox intact.
- [ ] **M2-08 🤖 `useSyncStatus`** — `state`, `pending`, `failed`, `lastSyncedAt`. **Done when:** values are correct through an offline → online cycle.
- [ ] **M2-09 🤖 `410` reset and `426` upgrade** (D-13, D-15). **Done when:** a reset keeps the outbox; `upgradeRequired` fires.

**Server**

- [ ] **M2-10 🤖 Core handler** — Fetch-API `pull`/`push`, `auth`, `scope` on read **and** write, `validate` hooks (D-17). **Done when:** unit tests, including a scope-escape attempt that must fail.
- [ ] **M2-11 🤖 Postgres adapter + `migrate`** — `_version/_seq/_xid/_deleted` columns, triggers, `sync_mutations` table (D-11, D-12, D-16). **Done when:** the long-transaction test (a row committed late with a lower `_seq`) is not skipped.
- [ ] **M2-12 🤖 Idempotency** (D-11). **Done when:** replaying any push returns `duplicate` with the stored result and changes nothing.
- [ ] **M2-13 🤖 Prisma and Drizzle adapters + Express adapter**. **Done when:** `examples/server-express-postgres` runs from `docker compose up`.
- [ ] **M2-14 🤖 Conformance CLI** — `npx sync-flow test --url --token` with CI exit codes, covering every MUST in `spec/protocol.md`. **Done when:** green against the Express example and red against a deliberately broken one.

**Proof**

- [ ] **M2-15 🤖 Property tests** — the C++ core as a Node addon, 3 simulated devices, random writes/pulls/crashes/duplicates/clock skew (D-32). **Done when:** 10,000 runs converge in CI (nightly runs 100,000).
- [ ] **M2-16 🤖 Chaos + device e2e** — dropped connection mid-page, 30 s latency, 5xx bursts; Maestro: offline create → kill → relaunch → online → row on server. **Done when:** green on both platforms.
- [ ] **M2-17 👤 Gate B review**
  - **How:** run `docker compose up` in `examples/server-express-postgres` (the agent gives the exact command), point the example app at it, then on two phones edit the same list offline and go online. Check both phones end up identical and no edit is lost. Reply "Gate B done".

## M3 Agent-proof 0.1.0 (weeks 8–9)

**Goal:** an AI agent can add Sync Flow to an app on the first try. First real npm release.
**Gate C:** a fresh agent session, given only "add @nikhilll/sync-flow to this app with an Express backend" and the npm package, gets a blank Expo app syncing, with `doctor` passing and e2e green, without human help.

- [ ] **M3-01 🤖 Error catalogue** — every `SF_*` code with `fix` + `docs`, `docs/errors.md`, a CI check that no code is missing (D-27). **Done when:** the CI check exists and passes.
- [ ] **M3-02 🤖 `npx sync-flow init`** — non-interactive with flags, `--json`; writes schema, client, provider wiring and server route. **Done when:** run on a blank Expo app + Express app, the result builds.
- [ ] **M3-03 🤖 `npx sync-flow doctor --json`** — New Architecture, Nitro, prebuild state, plugin, server reachable, spec version. **Done when:** each failure case gives the right code and fix.
- [ ] **M3-04 🤖 Package `AGENTS.md`** — golden path word for word, invariants, mistakes → fixes, version pin (D-28). **Done when:** it's in the npm tarball (`npm pack --dry-run` shows it).
- [ ] **M3-05 🤖 Docs site** on GitHub Pages: quickstart, API, protocol, errors, FAQ. **Done when:** live at `8bits-nikk.github.io/sync-flow`.
- [ ] **M3-06 🤖 `llms.txt` + `llms-full.txt`** generated from `docs/` on release, also in the package. **Done when:** generated in CI.
- [ ] **M3-07 🤖 Type-checked docs** — every code block compiles in CI (D-29). **Done when:** a broken snippet fails CI.
- [ ] **M3-08 🤖 Agent Skill** `skills/sync-flow/SKILL.md`. **Done when:** it follows the Agent Skills format and passes a dry run.
- [ ] **M3-09 🤖 README** — 30-second pitch, install, 20-line example, link to docs, comparison table. **Done when:** reviewed by you (M3-12).
- [ ] **M3-10 🤖 Release workflow** — changesets, lockstep versions, `npm publish --provenance` via trusted publishing (D-31). **Done when:** a dry run in CI succeeds.
- [ ] **M3-11 🤖 Gate C run** — the agent records a fresh session doing the gate task and saves the transcript in `bench/agent-gate/`. **Done when:** passes without human help (if it fails, fix the docs or API, never the test).
- [ ] **M3-12 👤 Review the README and quickstart**
  - **How:** read the README as if you'd never seen the project. Does the first screen tell you what it is and how to start? Reply with anything unclear.
- [ ] **M3-13 👤 Approve the 0.1.0 release**
  - **How:** the agent opens the "Version Packages" PR made by changesets. Read the CHANGELOG, then merge it; the workflow publishes. Check npmjs.com shows 0.1.0 with a provenance badge.

## M3b Standalone server + pilot 0.2.0 (weeks 10–11)

**Goal:** any backend language gets sync by running our server. Our own pilot app proves it.
**Gate D:** `examples/pilot-fieldnotes` syncs through the standalone server with **zero** sync code in its backend, and conformance is green against the standalone server.

**Standalone server**

- [ ] **M3b-01 🤖 `sync-flow start` + YAML config** — load and validate config, publish a JSON Schema for editor autocomplete. **Done when:** a bad config gives a clear error with the line number.
- [ ] **M3b-02 🤖 `schema export`** → `schema.json` from the app's TS schema. **Done when:** the standalone server loads it.
- [ ] **M3b-03 🤖 JWT auth** — JWKS URL or shared secret, `userId` claim (D-18). **Done when:** tests for expired, wrong-signature and missing tokens.
- [ ] **M3b-04 🤖 Declarative `scope`** including `$user` and following `t.ref`. **Done when:** a scope-escape test fails as it should.
- [ ] **M3b-05 🤖 `webhook` write mode** — HMAC-SHA256 signature, approve / reject / approve+patch, timeouts and retries. **Done when:** tests for each decision and for a bad signature.
- [ ] **M3b-06 🤖 `direct` write mode** — `readonly`, `required`. **Done when:** tested.
- [ ] **M3b-07 🤖 `migrate --apply`** for standalone mode. **Done when:** runs safely twice (idempotent).
- [ ] **M3b-08 🤖 Realtime poke** — LISTEN/NOTIFY → SSE, client pulls on poke (D-19). **Done when:** a change on phone A appears on phone B in under 2 s on a LAN.
- [ ] **M3b-09 🤖 `/health`, `/metrics`, structured logs without row data**. **Done when:** a test greps the logs for row values and finds none.
- [ ] **M3b-10 🤖 Docker image** on GHCR, multi-arch (amd64 + arm64), tags `0`, `0.2`, `0.2.x` (D-31). **Done when:** `docker run ghcr.io/8bits-nikk/sync-flow-server:0` starts.

**Pilot**

- [ ] **M3b-11 🤖 `examples/pilot-fieldnotes`** — notes with photo URLs, checklists, lists shared between 2 users; standalone server + Postgres in docker compose; a tiny webhook backend (D-33). **Done when:** `docker compose up` + `npx expo run:android` works from the README.
- [ ] **M3b-12 🤖 `examples/server-dotnet`** — a .NET webhook backend only (proves "any language"). **Done when:** the pilot works with it swapped in.
- [ ] **M3b-13 🤖 Pilot e2e** — Maestro two-device flows: offline edits, a shared-row conflict, a delete vs edit. **Done when:** green in CI.
- [ ] **M3b-14 👤 Try the pilot yourself**
  - **How:** the agent gives one command to run the stack (it can run on this server under Docker) plus an APK. Install it on your phone and on a second phone or emulator, make notes offline on both, go online and check they merge. Reply with anything that felt wrong.
- [ ] **M3b-15 👤 Approve the 0.2.0 release** — same steps as M3-13, and also check the GHCR image page.

## M4 0.5 (weeks 12–16)

**Goal:** the features real apps ask for next, and proof in daily use.
**Gate:** the pilot is in daily use for 2 weeks with no data-loss bugs.

- [ ] **M4-01 🤖 `version` conflict mode + `onConflict`** (D-14).
- [ ] **M4-02 🤖 Background sync** — BGTaskScheduler / WorkManager via the config plugin option.
- [ ] **M4-03 🤖 Fastify, Nest and fetch adapters** (Hono, Next.js, Bun, Deno, Workers).
- [ ] **M4-04 🤖 MySQL** — library and standalone, counter-row `_seq` (D-12, D-16).
- [ ] **M4-05 🤖 Knex adapter**.
- [ ] **M4-06 🤖 TTL / eviction** of old synced rows on the device.
- [ ] **M4-07 🤖 Dev panel** — in-app overlay showing the outbox, failed items and the last sync.
- [ ] **M4-08 🤖 `/testing` kits** — in-memory test server and network chaos helpers for users' own tests.
- [ ] **M4-09 🤖 Certificate pinning** (optional config).
- [ ] **M4-10 🤖 Full benchmarks** vs op-sqlite + custom sync, WatermelonDB and PowerSync on real devices; publish results in `bench/` and the docs.
- [ ] **M4-11 🤖 Initial-sync perf** — 100k rows in < 8 s, no JS frame > 16 ms, < 60 MB peak (PLAN §10).
- [ ] **M4-12 👤 Daily use for 2 weeks**
  - **How:** use the pilot on your phone every day. If anything looks wrong (a missing or duplicated note, an edit that came back), send a screenshot and the time. The agent pulls the logs and opens an issue.
- [ ] **M4-13 👤 Approve the 0.5.0 release** — same as M3-13.

## M5 1.0 and later

- [ ] **M5-01 👥 API freeze review** — you approve the final 1.0 API list; after that, only additive changes until 2.0.
- [ ] **M5-02 🤖 WASM build of the core** for web and desktop.
- [ ] **M5-03 🤖 Blob/file sync**.
- [ ] **M5-04 🤖 CDC (logical replication) source** for the standalone server.
- [ ] **M5-05 🤖 Opt-in CRDT text columns**.
- [ ] **M5-06 👤 Approve the 1.0.0 release**.

---

## Change log of this file

| Date | Change |
|---|---|
| 2026-11-20 | Created from PLAN.md v2.1 §11 (92 tasks) |
