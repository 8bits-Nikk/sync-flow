# Sync Flow — Decisions

Every settled design decision, with a short reason. Read this **before** writing code, and don't reverse a decision in code.
To change one, add a new entry that supersedes it (and update [PLAN.md](PLAN.md)).

**How to use this file (for AI coding agents)**

- Each entry has a **Rule** line: that is what your code must do. The **Why** tells you the intent, so you can handle cases the rule doesn't name.
- **Rejected** lists options that were already considered. Don't bring them back as a "simpler" or "faster" alternative.
- If a task seems to need something that breaks a rule, stop and report it. Don't work around it.
- IDs are stable. Cite them in commit messages and PR descriptions, e.g. `feat(outbox): coalesce update+delete (D-21)`.

Status values: **Accepted** (in force), **Superseded by D-x**, **Proposed** (needs owner approval).

## Index

| ID | Area | Decision |
|---|---|---|
| D-01 | Scope | Free, MIT, npm + GitHub only |
| D-02 | Scope | Two packages, one peer dependency |
| D-03 | Platform | Expo SDK 55+ / RN 0.83+, New Architecture only, no Expo Go |
| D-04 | Native | Nitro Modules with a C++ core |
| D-05 | Native | Platform glue in Swift and Kotlin; no third-party RN packages |
| D-06 | Storage | SQLite3MultipleCiphers, encrypted by default |
| D-07 | Storage | One database file per user, wiped on sign-out |
| D-08 | Protocol | gzip JSON over HTTPS, two endpoints |
| D-09 | Protocol | yyjson parses in C++, off the JS thread |
| D-10 | Protocol | UUIDv7 ids generated on the device |
| D-11 | Protocol | Idempotent push via a `sync_mutations` table |
| D-12 | Protocol | `_seq` change cursor with a Postgres `xid8` horizon |
| D-13 | Protocol | Soft deletes + tombstone window + `410 RESET_REQUIRED` |
| D-14 | Protocol | Field-level merge by default; `version` mode is opt-in |
| D-15 | Protocol | Schema version on every request; `426 UPGRADE_REQUIRED` |
| D-16 | Server | Postgres first, MySQL later |
| D-17 | Server | One server package, used as a library or as a standalone program |
| D-18 | Server | Standalone writes via signed webhook (default) or direct rules |
| D-19 | Server | Realtime is a "poke" only, never data |
| D-20 | Engine | One writer thread, a reader pool, JS never waits on the network |
| D-21 | Engine | Outbox in the same DB file; coalesce only queued entries |
| D-22 | Engine | Quarantine after 8 attempts, blocking only the same row |
| D-23 | Engine | Push before pull; rebase local pending writes on pull |
| D-24 | Engine | Auth errors pause sync and never drop the outbox |
| D-25 | API | One way to do each thing; types inferred, no codegen |
| D-26 | API | Raw SQL is read-only |
| D-27 | API | Errors carry `code`, `fix` and `docs` |
| D-28 | Docs | AGENTS.md + llms.txt ship inside the npm package |
| D-29 | Docs | Every doc code block is type-checked in CI |
| D-30 | Tooling | pnpm workspaces, changesets, tsup, CMake; no Turborepo |
| D-31 | Release | npm trusted publishing with provenance; image on GHCR |
| D-32 | Testing | Property tests run the C++ core as a Node addon in CI |
| D-33 | Pilot | Our own dummy app, `examples/pilot-fieldnotes` |
| D-34 | Scope | Not in 1.0: web, P2P, CRDT everywhere, blobs, hosted cloud |

---

## Scope and packaging

### D-01 Free, MIT, npm + GitHub only
**Accepted** 2026-11-16 · Owner requirement

- **Rule:** Everything is MIT. Publish only to npm and to GitHub (repo, Pages for docs, GHCR for the Docker image). No hosted service, no paid tier, no telemetry, no licence keys, no "pro" code paths.
- **Why:** The owner wants a community package, not a product. Fair-source or hosted-only competitors (and Realm Device Sync's shutdown in Sep 2025) are exactly the lock-in this project avoids.
- **Rejected:** a paid cloud tier; BSL or fair-source licences.

### D-02 Two packages, one peer dependency
**Accepted** 2026-11-20 · Supersedes the v1 plan's 4 packages

- **Rule:** The app installs `@nikhilll/sync-flow` + `react-native-nitro-modules`. The backend installs `@nikhilll/sync-flow-server`. Nothing else is required. New features go into one of these two packages (as a subpath export if needed), never into a new package.
- **Why:** P15: developers drown in packages (driver + NetInfo + secure-store + background task + outbox + server code). "Don't overload with packages" was an explicit owner requirement.
- **Rejected:** separate `-expo`, `-react`, `-testing` or `-cli` packages; op-sqlite, NetInfo, expo-secure-store or expo-background-task as peers.
- **Consequence:** subpaths `@nikhilll/sync-flow/schema`, `/testing` and server `/postgres`, `/prisma`, `/drizzle`, `/knex`, `/nest`, `/testing`.

### D-34 Not in 1.0
**Accepted** 2026-11-20

- **Rule:** Don't build web or desktop clients, peer-to-peer sync, CRDTs on every column, file/blob sync or a hosted cloud before 1.0. Keep the C++ core free of iOS/Android-only APIs so a WASM build stays possible later.
- **Why:** Each one is a large project by itself. Mobile offline-first done right is the whole value of 0.x.

## Platform and native code

### D-03 Expo SDK 55+ / React Native 0.83+, New Architecture only
**Accepted** 2026-11-20 · Owner approved

- **Rule:** Support iOS 15.1+ and Android API 24+ on the New Architecture. Expo works through dev builds (`expo prebuild` / `expo run:*`). In Expo Go, throw `SF_EXPO_GO` with the fix command.
- **Why:** Expo dropped the Legacy Architecture in SDK 55, and Nitro/JSI needs the New Architecture. One supported architecture halves the test matrix.
- **Rejected:** a Legacy Architecture bridge fallback; a pure-JS mode for Expo Go.

### D-04 Nitro Modules with a C++ core
**Accepted** 2026-11-20

- **Rule:** Engine logic (storage, outbox, sync loop, JSON, change tracking) is C++ in `packages/sync-flow/cpp/`, exposed through Nitro HybridObjects. Bindings are generated by Nitrogen and committed in `nitrogen/`.
- **Why:** The owner asked for MMKV-class speed. Nitro gives synchronous JSI calls (needed for the synchronous first read, D-20) with typed codegen, and it's what MMKV v4 uses. One C++ core works on both platforms and can later be compiled to WASM (D-34).
- **Rejected:** a TurboModule written by hand (more boilerplate, weaker typing); Expo Modules API (Swift/Kotlin only, no shared C++ core); a Rust core (adds a second toolchain to every contributor's machine).

### D-05 Platform glue in Swift and Kotlin
**Accepted** 2026-11-20

- **Rule:** Only these things are platform code: HTTP (URLSession / OkHttp), key storage (Keychain / Android Keystore), reachability (NWPathMonitor / ConnectivityManager), background tasks (BGTaskScheduler / WorkManager). Response bytes go straight to C++, never through JS.
- **Why:** These APIs exist only on the platform side, and owning them removes four third-party peer packages (D-02). OkHttp already ships with React Native, so nothing is added to the APK.
- **Rejected:** `fetch` from JS (puts parsing and network on the JS thread, P5); libcurl in C++ (big binary, breaks system proxy and ATS rules).

## Storage

### D-06 SQLite3MultipleCiphers, encrypted by default
**Accepted** 2026-11-20 · Supersedes SQLCipher from v1

- **Rule:** Vendor the SQLite3MultipleCiphers amalgamation. Use AES-256 with the SQLCipher-v4-compatible scheme. Encryption is on by default; `encryption: false` exists for debugging only. Pragmas: WAL, `synchronous=NORMAL`, `foreign_keys=OFF`.
- **Why:** MIT licensed with built-in crypto, so no OpenSSL to bundle and a smaller binary than SQLCipher. Phones get lost, so data at rest must be encrypted without the developer having to remember. `foreign_keys=OFF` because rows arrive in pull order, not dependency order; the server owns relational integrity.
- **Rejected:** SQLCipher (needs OpenSSL/BoringSSL); op-sqlite as a dependency (D-02); an unencrypted default.

### D-07 One database file per user, wiped on sign-out
**Accepted** 2026-11-20

- **Rule:** File name `sf_<sha256(userId)[0:16]>.db`, each with its own 256-bit key in the Keychain (`AfterFirstUnlockThisDeviceOnly`) or wrapped by an Android Keystore key. `signOut()` refuses with `SF_PENDING_WRITES` while the outbox isn't empty; `signOut({ force: true })` deletes the file, WAL and key.
- **Why:** P13 (the next user on a shared device sees old rows) and P12 (users log out with unsynced work). A separate file makes the wipe total and simple. `AfterFirstUnlock` lets background sync open the DB while the phone is locked.
- **Rejected:** a `user_id` column filter in one shared DB (one missed `WHERE` leaks data).

## Wire protocol

### D-08 gzip JSON over HTTPS, two endpoints
**Accepted** 2026-11-20 · Supersedes MessagePack + zstd from v1

- **Rule:** `POST {url}/pull` and `POST {url}/push` only, JSON bodies with standard `Content-Encoding: gzip`. Headers on every request: `Authorization`, `X-SyncFlow-Client`, `X-SyncFlow-Schema`, `X-SyncFlow-Protocol: 1`. The normative spec is `spec/protocol.md` + `spec/openapi.yaml`.
- **Why:** Any backend in any language can implement two JSON endpoints in a day (P7). With parsing in C++ (D-09) JSON is fast enough, so a binary format would add work for every backend for little gain.
- **Rejected:** MessagePack, zstd, Protobuf, gRPC, WebSockets for data.

### D-09 yyjson parses in C++, off the JS thread
**Accepted** 2026-11-20

- **Rule:** Pull responses are parsed by yyjson on a native worker and applied in batches of 500 rows on the writer thread. JS receives only a "tables changed" event.
- **Why:** P5: JSON parsing on the JS thread freezes the UI during a large initial sync. yyjson is MIT, a single C file and among the fastest JSON parsers.
- **Rejected:** simdjson (larger, needs CPU feature checks per ABI); `JSON.parse` in JS.

### D-10 UUIDv7 ids generated on the device
**Accepted** 2026-11-16

- **Rule:** Row ids (`t.id()`) and mutation ids are UUIDv7, generated in C++. Apps never generate ids themselves; `insert()` returns the id.
- **Why:** Offline inserts need an id before the server sees them. UUIDv7 is time-ordered, so B-tree indexes stay compact, unlike random v4.
- **Rejected:** server-assigned integer ids (impossible offline); UUIDv4 (index fragmentation).

### D-11 Idempotent push via `sync_mutations`
**Accepted** 2026-11-16 · MUST

- **Rule:** The server stores `sync_mutations(id PK, user_id, applied_at, status, result_json)` and writes it in the same transaction as the mutation. A repeated `id` returns the stored result with `status: "duplicate"`. Keep records 30 days (≥ the client's maximum outbox age). An `insert` for an existing `rowId` is an upsert.
- **Why:** P3: the server applied a write, the reply was lost, the phone retries, and the order is created twice. This makes every retry safe.
- **Rejected:** idempotency by row content hashing; trusting the client not to retry.

### D-12 `_seq` change cursor with a Postgres `xid8` horizon
**Accepted** 2026-11-20 · MUST

- **Rule:** Every synced row has `_seq bigint` set by a database trigger (generated by `sync-flow migrate`) on insert, update and soft delete. On Postgres, the trigger also stores `_xid = pg_current_xact_id()`, and a pull returns rows with `_seq > cursor AND _xid < pg_snapshot_xmin(pg_current_snapshot())`. Other databases use a counter row updated inside the writing transaction. The cursor is opaque base64 of `{ seq, tombstoneEpoch }`. **Never** use `updated_at` or any client clock for change detection.
- **Why:** P9: a long transaction can commit a row with a lower sequence after a client already pulled past it, and `updated_at` cursors skip it for ever. The `xid8` horizon holds back rows from still-open transactions. Triggers mean writes from admin panels and scripts sync too.
- **Rejected:** `updated_at` timestamps; logical replication / CDC (planned for later, phase 5, as an optional source).

### D-13 Soft deletes, tombstone window, `410 RESET_REQUIRED`
**Accepted** 2026-11-16

- **Rule:** Synced tables have `_deleted boolean`; deletes set it and advance `_seq`. Purge tombstones after `tombstoneDays` (default 90). A client whose cursor is older than the purge horizon gets `410 { error: "RESET_REQUIRED" }`; it wipes synced rows, **keeps its outbox** and pulls from `null`.
- **Why:** P8: hard deletes are invisible to a cursor, so deleted rows reappear on other devices.

### D-14 Field-level merge by default; `version` mode opt-in
**Accepted** 2026-11-20

- **Rule:** Updates send only changed fields (`patch`) and a `baseVersion`. In `merge` tables (default) the server applies the patch onto the current row and bumps `_v`. In `version` tables a stale `baseVersion` returns `conflict` and the client's `onConflict` decides (`'server'` default, `'retry'` or a merged patch). Delete beats update (`ROW_DELETED`).
- **Why:** P2: whole-row last-write-wins silently loses edits to different fields. Field merge fixes most real cases with zero code; strict versioning is there for money-like data.
- **Rejected:** whole-row LWW; CRDTs for every column (D-34); vector clocks (too complex for app developers to reason about).

### D-15 Schema version on every request; `426 UPGRADE_REQUIRED`
**Accepted** 2026-11-16

- **Rule:** `defineSchema({ version })` is bumped only for destructive changes. The server returns only the columns that exist in the client's version and answers `426` below `minSchemaVersion`. Additive local changes migrate automatically; a version bump flushes the outbox, then resets and resyncs.
- **Why:** P10: installed apps can't be updated instantly, so the server must serve old and new apps at the same time.

## Server

### D-16 Postgres first, MySQL later
**Accepted** 2026-11-20 · Owner approved

- **Rule:** Phases 2–3b support Postgres only (library and standalone). MySQL arrives in phase 4. This is about the **server's** database; the app always uses its own SQLite (D-06).
- **Why:** Postgres has `xid8` snapshots (D-12) and LISTEN/NOTIFY (D-19), so the correct design is also the simplest there. Most Node and Prisma users are on Postgres.

### D-17 One server package, library or standalone
**Accepted** 2026-11-20 · Owner request

- **Rule:** `@nikhilll/sync-flow-server` has one core handler (Fetch-API `Request → Response`). Framework adapters (Express, Fastify, Nest, fetch) and the standalone `start` command (YAML config, Docker image) are thin wrappers around the same core. Never fork logic between the two modes.
- **Why:** Non-Node teams (.NET, Java, Go, Python, PHP) get sync with zero code by running the standalone server, and Node teams can embed it. One core means one set of bugs and one conformance suite.
- **Rejected:** a separate sync-service product with its own protocol (v1 plan); per-language server SDKs.

### D-18 Standalone writes: signed webhook (default) or direct rules
**Accepted** 2026-11-20

- **Rule:** `writes.mode: webhook` POSTs each mutation to the team's backend with an HMAC-SHA256 signature; the backend answers `approve`, `reject` + code, or `approve` + modified `patch`, and the sync server writes the DB. `writes.mode: direct` writes using only `scope` + `readonly`/`required` rules. Idempotency and `_seq` stay with the sync server in both modes. Auth is JWT via JWKS or a shared secret; the server never stores users.
- **Why:** Business rules belong in the team's backend, but sync mechanics shouldn't. A webhook is the smallest thing any language can implement. `direct` covers prototypes with no backend.
- **Rejected:** letting the backend write the DB itself during sync (breaks idempotency and the cursor guarantees).

### D-19 Realtime is a "poke" only
**Accepted** 2026-11-20 · Phase 3b

- **Rule:** Postgres LISTEN/NOTIFY → Server-Sent Events carrying no row data, only "something changed". The client reacts by running a normal pull.
- **Why:** There's one data path (pull) to make correct and secure. SSE works through proxies and needs no extra protocol.
- **Rejected:** WebSockets streaming rows (a second data path with its own ordering and auth problems).

## Client engine

### D-20 One writer thread, a reader pool, JS never waits on the network
**Accepted** 2026-11-20

- **Rule:** One serial writer thread does all local writes, outbox updates, pull apply and rebase. A pool of 2–4 readers re-runs queries. The first read of a `useQuery` is synchronous on JS through JSI (dev builds warn with `SF_SLOW_SYNC_READ` above 16 ms). Network calls never block JS.
- **Why:** SQLite allows one writer and many WAL readers, so a single writer removes locking bugs. A synchronous first read gives MMKV-style rendering with no loading flash (goal 2).

### D-21 Outbox in the same DB file; coalesce only queued entries
**Accepted** 2026-11-20

- **Rule:** `_sf_outbox` and `_sf_meta` live in the user's encrypted file. Each local write updates the user table and the outbox in one transaction. Coalesce only `queued` entries: insert+update → insert; insert+delete → remove both; update+update → merged patch; update+delete → delete. Never touch `inflight` entries.
- **Why:** P6: one file + one transaction means a crash can never lose a write or leave the row and the outbox disagreeing. Coalescing keeps uploads small after many offline edits; skipping `inflight` entries avoids racing a request already on the wire.

### D-22 Quarantine after 8 attempts, blocking only the same row
**Accepted** 2026-11-20

- **Rule:** Retry with backoff 1 s × 2ⁿ ± 20% jitter, capped at 5 min, for network errors and 5xx. A per-mutation `rejected` never retries. After 8 attempts the entry becomes `failed`: later entries **for the same row** wait, all other rows continue. Show it in `useSyncStatus().failed`; resolve with `db.outbox.retry(id)` / `discard(id)`.
- **Why:** P4: one bad write must not freeze every later upload, but writes to the same row must stay in order or they'd apply on the wrong base.

### D-23 Push before pull; rebase pending writes on pull
**Accepted** 2026-11-20

- **Rule:** Each cycle: push until the outbox is empty or offline, then pull until `hasMore=false`. Each pull page is one transaction that also saves the cursor. For a row with pending outbox entries, write the server row then re-apply the pending patches. The pending flag clears only when the pulled cursor reaches the push's write-checkpoint `cursor`.
- **Why:** The user must never see their own edit flicker back to the old value while it's in flight. The write checkpoint guarantees the pulled state already includes the write.

### D-24 Auth errors pause sync and never drop the outbox
**Accepted** 2026-11-16

- **Rule:** Native code calls `getToken()` through a Nitro callback and caches the token until a 401 or JWT `exp`. On 401: refresh once, retry once, then set `state: 'unauthorized'` and pause. `getToken()` returning null stops sync. The outbox is never dropped for auth reasons.
- **Why:** P11: tokens expire while offline, and some libraries lose queued writes when that happens.

## API and developer experience

### D-25 One way to do each thing; types inferred, no codegen
**Accepted** 2026-11-20 · MUST

- **Rule:** Exactly the API in PLAN.md §4: `defineSchema`/`table`/`t.*`, `createSyncFlow`, `SyncFlowProvider`, `useQuery`, `useRow`, `useSyncStatus`, `db.<table>.insert/update/delete/get/find/where`, `db.transaction`, `db.sync`, `db.signOut`, `db.outbox.retry/discard`. No aliases, no behaviour-changing overloads, no "advanced mode" options in 0.x. No codegen step and no Babel plugin; all types come from `typeof schema`.
- **Why:** P17: agents (and people) mix up APIs when there are several ways to do something. Inferred types turn typos into compile errors. Every build step is one more thing to get wrong.
- **Rejected:** decorators/model classes (WatermelonDB style); a codegen CLI; an ORM-style query language.

### D-26 Raw SQL is read-only
**Accepted** 2026-11-20

- **Rule:** `db.sql\`...\`` is allowed in `useQuery` for reads. Any write through it throws `SF_RAW_WRITE`. Tables read are detected with `sqlite3_set_authorizer`.
- **Why:** A raw write would bypass the outbox and never sync, which is silent data loss.

### D-27 Errors carry `code`, `fix` and `docs`
**Accepted** 2026-11-20 · MUST

- **Rule:** Every thrown error is a `SyncFlowError` with `code` (`SF_*`), `message`, `fix` (the exact command or code change) and a `docs` URL. Each code is listed in `docs/errors.md`. Adding an error without a catalogue entry fails CI.
- **Why:** An agent that sees `fix: "Run npx expo prebuild"` fixes the problem; one that sees "native module not found" guesses.

## Docs and agents

### D-28 AGENTS.md + llms.txt ship inside the npm package
**Accepted** 2026-11-20 · Owner requirement

- **Rule:** `packages/sync-flow/AGENTS.md` (golden path word for word, invariants, mistakes → fixes table, exact version) and `llms.txt` go in the npm tarball. The repo-root `AGENTS.md` is for agents working **on** this repo. `llms.txt`/`llms-full.txt` are generated from `docs/` on release. `skills/sync-flow/SKILL.md` is the Agent Skill.
- **Why:** An agent in a user's project reads `node_modules`, and the docs there always match the installed version.

### D-29 Every doc code block is type-checked in CI
**Accepted** 2026-11-20

- **Rule:** Code blocks in `docs/`, both AGENTS.md files and the README compile against the current source in CI. Examples run in e2e CI.
- **Why:** Docs that drift from the API teach agents wrong code, which defeats D-25 and D-28.

## Tooling, release and testing

### D-30 pnpm workspaces, changesets, tsup, CMake; no Turborepo
**Accepted** 2026-11-20

- **Rule:** pnpm workspaces, changesets for versioning (both packages in lockstep, same version), tsup for TS builds, Nitrogen for bindings, CMake for C++ (GoogleTest for C++ unit tests), GitHub Actions for CI, Maestro for device e2e.
- **Why:** Standard, free tools. Turborepo adds config for two packages and no benefit.

### D-31 npm trusted publishing with provenance; image on GHCR
**Accepted** 2026-11-20

- **Rule:** Release from GitHub Actions with npm trusted publishing (OIDC) and `--provenance`. No npm token stored in secrets. The Docker image goes to `ghcr.io/8bits-nikk/sync-flow-server` with tags `0`, `0.x`, `0.x.y`.
- **Why:** No long-lived secret to leak, and users can verify that the tarball was built from the repo. GHCR is free for public images.

### D-32 Property tests run the C++ core as a Node addon in CI
**Accepted** 2026-11-20

- **Rule:** CI builds the same C++ core as a Node addon (test-only, never published) and runs property tests: random offline writes, pulls, crashes, duplicate deliveries and clock skew across 3 simulated devices must always converge to the server state.
- **Why:** P14: sync bugs live in rare interleavings. Thousands of random runs on a CI machine find them; device tests can't.

### D-33 Our own dummy pilot app
**Accepted** 2026-11-20 · Owner approved

- **Rule:** `examples/pilot-fieldnotes`: an Expo app with notes (photos as URLs), checklists and lists shared between 2 users, running against the standalone server + Postgres in docker compose, with Maestro e2e. It is the gate for 0.2 (gate D) and 0.5.
- **Why:** It exercises offline edits, shared rows, conflicts and deletes, and proves the zero-sync-code backend story.
