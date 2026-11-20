# Sync Flow — Plan

> Offline-first data for React Native and Expo, with the sync logic done for you.
> A JSI/C++ core (Nitro Modules, like MMKV v4) that works with any backend.
> MIT licensed and free: npm + GitHub only. No hosted service, no paid tier.

| | |
|---|---|
| Status | Plan v2.1 (2026-11-20): owner decisions in §12 and the standalone server in §4.6 added. Supersedes the v1 plan in Claude Docs ("Offline-First Sync — Implementation Plan", 2026-11-16) |
| Owner | @nikhilll (npm), 8bits-Nikk (GitHub) |
| Packages | `@nikhilll/sync-flow` (app), `@nikhilll/sync-flow-server` (backend). Both names are free on npm |
| Repo | `github.com/8bits-Nikk/sync-flow` (name free) |
| Licence | MIT for everything |

**Companion files:** [DECISIONS.md](DECISIONS.md) (every decision with its reason and the rule code must follow) and [MILESTONES.md](MILESTONES.md) (the task checklist and progress).

**How to read this file:** sections 1–3 explain *why*. Sections 4–9 are the *contract*: API, protocol, storage and the rules that implementation must follow. Sections 10–12 are *how and when*. A coding agent implementing a milestone should read sections 4–9 completely before writing code. Anything marked **MUST** is a hard requirement, and anything marked **Decision** is settled and should not be changed without updating this file.

---

## 1. The problem: why offline-first is still hard

The research (sources in section 13) agrees on one point: **storage is solved, sync is not.** Teams pick a local database, then spend months on the parts nobody ships for them.

| # | What developers struggle with | What happens in practice |
|---|---|---|
| P1 | Sync is left to you. WatermelonDB gives push/pull primitives only, and you build the protocol, backend and retries yourself | Months of custom work. Most teams get edge cases wrong |
| P2 | Lost updates from naive last-write-wins | Two people edit the same record offline, and the later sync silently overwrites the other |
| P3 | Duplicate writes when a request is retried after a timeout | The server applied it, the phone never got the reply, so the phone sends it again and the order is created twice |
| P4 | One bad queued write blocks the whole queue | A single rejected mutation stops every later upload |
| P5 | The UI freezes during sync | JSON parsing and row applying run on the JS thread |
| P6 | The app is killed in the middle of a sync | Half-applied pages, corrupted cursors, lost queued writes |
| P7 | Backend lock-in | PowerSync needs its own service, Zero needs Postgres and TypeScript, Electric syncs reads only. Realm Device Sync was shut down on 30 Sep 2025, which stranded its users |
| P8 | Deletes and tombstones get forgotten | Deleted rows come back, or never disappear on other devices |
| P9 | Change cursors based on `updated_at` miss rows | Clock skew and long transactions mean a row committed late with an older timestamp is skipped for ever |
| P10 | Schema changes break app versions already installed | Old apps crash on new columns, and new apps on old data |
| P11 | The token expires while offline | The sync fails with a 401 and some libraries drop the queue |
| P12 | Users can't tell that data is still uploading | They log out or uninstall with unsynced work |
| P13 | Data leaks on shared devices | The next user sees the previous user's rows after logout |
| P14 | It's hard to test | Airplane mode is not enough. Flapping networks, killed apps and clock skew are rarely tested |
| P15 | Too many packages to wire together | A typical setup is SQLite driver + NetInfo + secure-store + background-task + query cache + outbox library + backend code |
| P16 | TanStack Query "offline" is a cache, not a database | Paused mutations need extra persister setup, failed mutations are not persisted, and the cache is restored as a snapshot |
| P17 | AI agents get it wrong | Many options and several ways to do one thing, so agents mix APIs and invent config |

**Sync Flow exists to make every row of this table someone else's solved problem.** Section 3 maps each one to the feature that handles it.

## 2. Goals and non-goals

**Goals**

1. **One install, one setup file.** Developers install one package in the app and one in the backend. There is one required peer dependency (`react-native-nitro-modules`), and no NetInfo, secure-store, background-task or SQLite driver to add.
2. **Instant UI.** Reads and writes hit the local database; the network is never on the UI path. The first render of a query can be synchronous, MMKV-style, so there is no loading flash.
3. **Sync runs off the JS thread.** HTTP, JSON parsing, applying rows and rebasing all happen in native C++ worker threads.
4. **Works with any backend.** The wire protocol is two JSON endpoints. Node gets a drop-in handler; every other language implements the spec, and a conformance CLI proves it is correct.
5. **Correct by default.** Idempotent writes, ordered outbox, quarantine instead of blocking, field-level merge, a safe change cursor, tombstones, crash-safe checkpoints.
6. **Secure by default.** Encrypted at rest, one database per user, wiped on sign-out. The server enforces access.
7. **Agent-proof docs and API.** One way to do each thing, inferred types, error codes that say how to fix the problem, `AGENTS.md` + `llms.txt` shipped inside the npm package.
8. **Free.** MIT, npm and GitHub only. Any server piece we ship is on npm, or as a Docker image on GitHub Container Registry.

**Non-goals for 1.0**

- Expo Go. JSI native code needs a dev build (`npx expo prebuild` or `expo run:*`), and the error message says so.
- Web and desktop clients. The C++ core stays portable so a WASM build can follow (phase 5).
- Peer-to-peer sync, and CRDTs everywhere. Opt-in CRDT text columns are a later item.
- A hosted cloud. The standalone server (§4.6) is self-hosted only: Docker on GHCR or `npx`.
- File/blob sync. Phase 5; until then, store a file URL in a row.

## 3. Pain points → solutions

| Pain | Sync Flow answer | Where |
|---|---|---|
| P1 | The full protocol, client engine and Node server kit ship together. `npx sync-flow init` writes the setup | §4, §5, §6 |
| P2 | Per-row `_version`; patches carry only changed fields (field-level merge by default); opt-in strict `version` conflicts with an `onConflict` hook | §5.4 |
| P3 | Every mutation has a client UUIDv7 `id`; the server records it in `sync_mutations` and returns `duplicate` on replay | §5.3 |
| P4 | Mutations that fail are quarantined after 8 attempts. Only later writes to **the same row** wait; others continue. It shows in `useSyncStatus().failed` | §7.3 |
| P5 | Native HTTP → C++ JSON parse (yyjson) → batched apply on a writer thread. JS only receives "tables changed" events | §7.1 |
| P6 | Each pull page is applied in one transaction together with its cursor. The outbox lives in the same SQLite file (WAL), so a crash loses nothing | §7.2 |
| P7 | A plain JSON protocol with any backend. The standalone server serves the endpoints for any backend language with no code; the Node kit embeds them; the OpenAPI spec and conformance CLI cover anyone building their own | §4.6, §5, §6 |
| P8 | Soft-delete column `_deleted` and a tombstone retention window; old clients get `resetRequired` | §5.5 |
| P9 | A server-assigned change sequence `_seq`, with a transaction-safe horizon (Postgres `xid8` snapshot), never client clocks | §5.6 |
| P10 | `schemaVersion` sent on every request; server returns only columns the client knows; `upgradeRequired` event for too-old apps | §5.7 |
| P11 | `getToken()` is called before requests; a 401 refreshes once, then pauses sync. The outbox is never dropped on auth errors | §7.4 |
| P12 | `useSyncStatus()` gives `pending`, `failed`, `lastSyncedAt` and `state`. `db.signOut()` refuses while there are pending writes unless `{ force: true }` | §4.4 |
| P13 | One encrypted database file per user, with keys in Keychain/Keystore; `signOut()` deletes the file and the key | §8 |
| P14 | Test kit: in-memory test server, network chaos, clock skew. Property tests in our CI | §10 |
| P15 | Two packages total, one peer dependency. Network status, background tasks and the keychain are built into our native code | §4.1 |
| P16 | A real database with a durable outbox, not a cache | §7 |
| P17 | One API shape, inferred types, error codes with a `fix`, `AGENTS.md` and `llms.txt` in the package, `doctor --json` | §9 |

### Competitive position (as of Nov 2026)

| | Sync Flow | PowerSync | WatermelonDB | ElectricSQL | Zero |
|---|---|---|---|---|---|
| Backend | Any (2 endpoints) | Own sync service + Postgres/Mongo/MySQL | Any, but you build the sync | Postgres | Postgres + TS server |
| Writes handled | Yes (outbox, retries, idempotency, rebase) | Yes | You build it | No (read path) | Yes |
| Server code shipped | Yes (Node kit + spec + conformance) | Service | No | Service | Service |
| Native core | C++ via Nitro (JSI) | Rust core + SQLite driver | JSI SQLite adapter | — | — |
| Licence | MIT | Fair-source parts (check current terms) | MIT | Apache-2.0 | Check current terms |
| Packages an app installs | 1 (+ nitro-modules) | Several | Several | Several | Several |

## 4. Developer experience (the golden path)

### 4.1 Install

```bash
# App (Expo or bare React Native, New Architecture)
npx expo install @nikhilll/sync-flow react-native-nitro-modules
npx expo prebuild            # or: npx expo run:android / run:ios
                             # bare RN: cd ios && pod install

# Backend (Node)
npm i @nikhilll/sync-flow-server
npx sync-flow migrate --dialect postgres --out ./migrations   # SQL for _version/_seq/_deleted + triggers
```

- The Expo config plugin is auto-registered via `app.plugin.js`. Adding `"plugins": ["@nikhilll/sync-flow"]` is needed only for options such as background sync.
- **Supported:** Expo SDK 55+ / React Native 0.83+ on the New Architecture. Legacy Architecture is not supported; Expo dropped it in SDK 55. iOS 15.1+, Android API 24+.

### 4.2 Define the schema once (shared by app and server)

```ts
// sync/schema.ts — pure TypeScript, safe to import in Node and in the app
import { defineSchema, table, t } from '@nikhilll/sync-flow/schema';

export const schema = defineSchema({
  version: 1,                       // bump only for destructive changes (rename/remove/type change)
  tables: {
    lists: table({
      id: t.id(),                   // UUIDv7 generated on the device
      name: t.text(),
      ownerId: t.text(),
    }),
    todos: table({
      id: t.id(),
      listId: t.ref('lists'),
      title: t.text(),
      done: t.bool().default(false),
      dueAt: t.date().optional(),
      note: t.text().optional(),
    }, {
      indexes: [['listId', 'dueAt']],
      conflict: 'merge',            // 'merge' (default) | 'version'
    }),
  },
});
export type Schema = typeof schema;
```

Column types: `t.id()`, `t.text()`, `t.int()`, `t.real()`, `t.bool()`, `t.date()` (stored as epoch ms), `t.json<T>()`, `t.ref(table)`. Modifiers: `.optional()`, `.default(v)`. **That is the whole list.** Types are inferred, with no codegen step.

### 4.3 Create the client

```ts
// sync/db.ts
import { createSyncFlow } from '@nikhilll/sync-flow';
import { schema } from './schema';

export const db = createSyncFlow({
  schema,
  url: 'https://api.example.com/sync',     // base URL; the SDK calls /pull and /push under it
  getToken: () => auth.getAccessToken(),   // async; return null when signed out
});
```

### 4.4 Use it

```tsx
import { SyncFlowProvider, useQuery, useRow, useSyncStatus } from '@nikhilll/sync-flow';
import { db } from './sync/db';

export default function App() {
  const user = useAuth();
  // Opens the user's encrypted database when userId is set, closes it when it becomes null.
  return (
    <SyncFlowProvider db={db} userId={user?.id ?? null}>
      <Todos listId="..." />
    </SyncFlowProvider>
  );
}

function Todos({ listId }: { listId: string }) {
  const { data: todos } = useQuery(
    db.todos.where({ listId, done: false }).orderBy('dueAt', 'asc').limit(50),
  );                                         // first render is synchronous; it re-runs only when `todos` changes
  const { pending, failed, state } = useSyncStatus();

  return (/* ... */ null);
}

// Writes are plain async calls: local and instant, queued for upload automatically.
const id = await db.todos.insert({ listId, title: 'Milk' });   // returns the new id
await db.todos.update(id, { done: true });                      // patch: only changed fields
await db.todos.delete(id);

await db.transaction(async (tx) => {                            // several writes, one local transaction
  const listId = await tx.lists.insert({ name: 'Groceries', ownerId: me });
  await tx.todos.insert({ listId, title: 'Eggs' });
});

await db.sync();                     // optional: force a sync now (it runs automatically)
await db.signOut();                  // throws SF_PENDING_WRITES if unsynced; signOut({ force: true }) wipes anyway
```

Read API (complete): `db.<table>.where(filter).orderBy(col, dir).limit(n).offset(n)`, `useQuery(q)`, `useRow(db.<table>, id)`, `db.<table>.get(id)`, `db.<table>.find(q)`, and the escape hatch `useQuery(db.sql\`SELECT ...\`)`. The engine detects the tables read automatically, and raw SQL is read-only: writes through `db.sql` throw `SF_RAW_WRITE`.

Filter operators: equality, `{ col: { gt, gte, lt, lte, ne, in, like } }`, `$or: [...]`. Nothing else.

### 4.5 Backend (Node)

```ts
// server/sync.ts
import { createSyncHandler } from '@nikhilll/sync-flow-server';
import { postgresAdapter } from '@nikhilll/sync-flow-server/postgres'; // or /prisma, /drizzle, /knex
import { schema } from '../sync/schema';

export const sync = createSyncHandler({
  schema,
  db: postgresAdapter(pool),
  auth: async (req) => {                                   // return null → 401
    const user = await verifyJwt(req.headers.get('authorization'));
    return user ? { userId: user.sub } : null;
  },
  scope: {                                                 // which rows each user may read and write
    lists: (u) => ({ ownerId: u.userId }),
    todos: (u) => ({ list: { ownerId: u.userId } }),       // via t.ref('lists')
  },
  validate: {                                              // optional business rules
    todos: async (m, u) => { if (m.patch.title === '') return reject('EMPTY_TITLE'); },
  },
});

// Pick one adapter for your framework:
app.use('/sync', sync.express());        // Express
// fastify.register(sync.fastify(), { prefix: '/sync' })
// SyncFlowModule.forRoot(sync)          // NestJS, from '@nikhilll/sync-flow-server/nest'
// export const POST = sync.fetch;       // Hono, Next.js route handlers, Bun, Deno, Workers
```

Non-Node backends have two options:

- Run the **standalone server** (§4.6), with no sync code in their own backend. This is the recommended option.
- Implement `spec/openapi.yaml` themselves, then run `npx sync-flow test --url https://their-api/sync --token ...` until all conformance cases pass.

### 4.6 Standalone server: sync for any backend, no code

**Decision (2026-11-20, owner request):** ship an all-in-one server. A team points it at their database and their auth, and it serves `/pull` and `/push` automatically. Their own backend can be any language (.NET, Java, Go, Python, PHP, Rails) and needs no sync code.

It is **the same `@nikhilll/sync-flow-server` package**, run as a program instead of imported as a library, so there is still nothing extra to learn:

```bash
# Option A: Docker (published free on GitHub Container Registry)
docker run -p 8787:8787 -v ./sync.config.yaml:/app/sync.config.yaml ghcr.io/8bits-nikk/sync-flow-server:0

# Option B: Node
npx @nikhilll/sync-flow-server start --config sync.config.yaml
```

```yaml
# sync.config.yaml — language-neutral; JSON Schema published for editor autocomplete
database:
  url: ${DATABASE_URL}                 # Postgres first; MySQL in 0.5
schema: ./schema.json                  # from the app: npx sync-flow schema export > schema.json
auth:
  jwksUrl: https://auth.example.com/.well-known/jwks.json   # or: secret: ${JWT_SECRET}
  userId: sub                          # JWT claim that identifies the user
scope:                                 # which rows each user may read and write
  lists: { ownerId: $user }
  todos: { list: { ownerId: $user } }  # follows t.ref('lists')
writes:
  mode: webhook                        # webhook | direct
  webhook:
    url: https://api.example.com/sync-webhook   # your backend, any language
    secret: ${WEBHOOK_SECRET}                   # HMAC-SHA256 signature header
  direct:                              # used when mode: direct (no backend at all)
    readonly: { todos: [ownerId] }     # columns clients may never set
    required: { todos: [title] }
realtime: true                         # Postgres LISTEN/NOTIFY → SSE "poke" so clients pull at once
```

**Write modes:**

| Mode | Who validates writes | What the team writes |
|---|---|---|
| `webhook` (recommended) | Their backend. The server POSTs each mutation, signed with HMAC. Their endpoint answers `{ "decision": "approve" }`, `{ "decision": "reject", "code": "…" }` or `{ "decision": "approve", "patch": {…} }` (modified). The sync server then writes the database, and idempotency and the `_seq` cursor stay handled for them | One small HTTP endpoint with business rules, in any language. No DB code and no sync logic |
| `direct` | The sync server, using `scope` plus the declarative `readonly`/`required` rules | Nothing; a fit for simple apps and prototypes |

**What it handles for the team:**

- Auth: JWT verification via JWKS or a shared secret; it has no user store of its own.
- `sync_mutations` idempotency.
- Change cursors (§5.6) and tombstones.
- `npx sync-flow migrate --apply` adds the `_version`/`_seq`/`_deleted` columns and triggers.
- Health check `GET /health` and Prometheus `/metrics`.
- Structured logs with no row data.
- Horizontal scaling: it is stateless, and everything lives in the database.

The Node library mode (§4.5) remains for teams that want sync inside their existing Node app.

## 5. Wire protocol v1 (the contract)

JSON over HTTPS with gzip (standard `Content-Encoding`), so any backend can implement it. **Decision:** no MessagePack or zstd in v1. gzip JSON plus a native C++ parser is fast enough and keeps every backend simple; this replaces the v1 plan's MessagePack + zstd.

All requests send `Authorization: Bearer <token>`, `X-SyncFlow-Client: <clientId>`, `X-SyncFlow-Schema: <schemaVersion>` and `X-SyncFlow-Protocol: 1`.

### 5.1 `POST {url}/pull`

```jsonc
// request
{ "cursor": "opaque-string-or-null", "limit": 1000 }
// response 200
{
  "changes": { "todos": [ { "id": "0193…", "title": "Milk", "done": false, "_v": 7 } ] },
  "deletes": { "todos": ["0193…"] },
  "cursor": "opaque",            // persist with the page, in the same transaction
  "hasMore": true                // keep pulling until false
}
```

- `cursor: null` means an initial sync. Paging is the same code path, so an initial sync can resume after a crash.
- Rows are filtered by `scope` for the authenticated user. The server never trusts client filters.
- Response `410` with `{ "error": "RESET_REQUIRED" }` means the client wipes synced data, keeps its outbox and pulls from `null`.

### 5.2 `POST {url}/push`

```jsonc
// request: at most 100 mutations, in outbox order
{ "mutations": [
  { "id": "0193…(uuidv7)", "table": "todos", "op": "update", "rowId": "0193…",
    "patch": { "done": true }, "baseVersion": 6 }
] }
// response 200: one result per mutation, same order
{ "results": [
  { "id": "0193…", "status": "applied", "row": { "...": "...", "_v": 7 } }
  // status: "applied" | "duplicate" | "rejected" | "conflict"
  // rejected → { "code": "EMPTY_TITLE", "row": <current server row or null> }
  // conflict → { "row": <current server row> }      (only for conflict: 'version' tables)
],
  "cursor": "opaque" }           // write checkpoint: the server state includes these writes from this cursor on
```

- `op` is one of `insert`, `update` or `delete`. An `insert` whose `rowId` already exists is treated as an `update` (upsert), which makes retries safe.
- The server applies each mutation in its own transaction, together with its `sync_mutations` record.

### 5.3 Idempotency (MUST)

- The server stores `sync_mutations(id PRIMARY KEY, user_id, applied_at, status, result_json)`.
- Before applying a mutation, the server looks up its `id`. If it is found, it returns the stored result with `status: "duplicate"`.
- Records are kept 30 days, configurable, and must be longer than the client's maximum offline outbox age, which is also 30 days by default.

### 5.4 Conflicts

| Table option | Server behaviour | Client behaviour |
|---|---|---|
| `merge` (default) | Applies only the patch fields onto the current row and bumps `_v`. Edits to different fields both survive; for the same field, the last one to reach the server wins | Nothing extra |
| `version` | If `baseVersion != current _v`, returns `conflict` with the current row and does not apply | Calls `onConflict({ local, server, mutation })`, which returns `'server'` (default), `'retry'` (re-send with the new baseVersion) or a merged patch |

- Delete vs update: delete wins. An update to a deleted row gets `rejected` with code `ROW_DELETED`.
- Rejected writes roll back locally: the engine writes the returned server row (or removes the row if `row` is null), then re-applies any later pending mutations for that row.

### 5.5 Deletes and tombstones

- Synced server tables get `_deleted boolean`. Deletes are soft deletes, and `_seq` advances on each.
- Tombstones are purged after `tombstoneDays`, default 90. A client whose cursor is older than the purge horizon gets `410 RESET_REQUIRED`.

### 5.6 Change cursor (MUST: never use client or `updated_at` clocks)

- Each synced row has `_seq bigint`, set by a trigger on every insert, update or soft delete. The trigger is generated by `npx sync-flow migrate`, so writes from admin panels and scripts sync too.
- **Postgres:** the trigger stores `_xid = pg_current_xact_id()`. A pull returns rows with `_seq > cursor` **and** `_xid < pg_snapshot_xmin(pg_current_snapshot())`, so rows from still-open transactions are never skipped.
- **MySQL / SQL Server / SQLite:** `_seq` comes from a single counter row that is updated inside the writing transaction. This serialises synced writes per database; that is fine for small and medium apps, and the limitation is documented.
- The cursor is opaque to the client: base64 of `{ seq, tombstoneEpoch }`.

### 5.7 Schema versions

- The client sends `X-SyncFlow-Schema`. The server returns only the tables and columns that exist in that version; it keeps a `schemaHistory` derived from the schema `version` + git history, or declares it explicitly.
- If the client is older than `minSchemaVersion`, the server answers `426 UPGRADE_REQUIRED`, and the app gets an `upgradeRequired` event to show "Please update".
- Locally, additive changes (new table, column or index) migrate automatically. A `version` bump means: flush the outbox if online, then reset synced data and resync. If offline, the reset waits.

## 6. Packages and repository

**Decision:** two npm packages, nothing else required. This replaces the v1 plan's 4 packages + a Docker image.

| Package | Contents | Runs on | Dependencies |
|---|---|---|---|
| `@nikhilll/sync-flow` | C++ core (SQLite3MultipleCiphers, sync engine, yyjson), Swift/Kotlin glue (HTTP, Keychain/Keystore, reachability, background tasks), TS API + React hooks, `/schema` subpath (pure TS), `/testing` subpath, Expo config plugin, `AGENTS.md`, `llms.txt` | iOS, Android (Expo dev builds and bare RN) | Peer: `react`, `react-native`, `react-native-nitro-modules` |
| `@nikhilll/sync-flow-server` | Library: Fetch-API handler + Express/Fastify/Nest/fetch adapters; DB adapters `/postgres`, `/prisma`, `/drizzle`, `/knex` (MySQL in P1); `/testing` (in-memory server). **Standalone server:** `start` command + YAML config + webhook/direct write modes (§4.6). CLI `sync-flow` (`init`, `migrate`, `schema export`, `doctor`, `test`, `start`); bundled `openapi.yaml` | Node 20+, Bun, Deno | `pg` is bundled for standalone mode; library mode uses the DB client the user already has (optional peer) |
| `ghcr.io/8bits-nikk/sync-flow-server` | Docker image of the standalone server (not npm; free on GHCR) | Any Docker host | — |

The schema builder is the same source file bundled into both packages. Each exports `defineSchema` from `/schema`, and the two are released in lockstep with the same version number.

```
sync-flow/
├── AGENTS.md                 # for agents working ON this repo
├── PLAN.md                   # this file
├── llms.txt                  # generated from docs/
├── spec/
│   ├── protocol.md           # §5, expanded, normative
│   └── openapi.yaml
├── packages/
│   ├── sync-flow/
│   │   ├── cpp/              # core: db/, engine/, outbox/, changes/, json/, crypto/ (+ tests/ with GoogleTest)
│   │   ├── ios/              # Swift: HTTP (URLSession), Keychain, NWPathMonitor, BGTaskScheduler
│   │   ├── android/          # Kotlin: HTTP (OkHttp, already in RN), Keystore, ConnectivityManager, WorkManager
│   │   ├── nitrogen/         # Nitro-generated bindings (committed)
│   │   ├── src/              # TS: createSyncFlow, query builder, hooks, schema/, testing/
│   │   ├── app.plugin.js     # Expo config plugin
│   │   ├── AGENTS.md         # for agents USING the package (shipped in the npm tarball)
│   │   └── llms.txt
│   └── sync-flow-server/
│       ├── src/              # core handler, adapters/, frameworks/, cli/, testing/
│       └── AGENTS.md
├── examples/
│   ├── pilot-fieldnotes/     # THE pilot: Expo SDK 57 app + standalone server + Postgres (docker compose), e2e with Maestro
│   ├── expo-todo/            # minimal golden-path app used in docs
│   ├── server-express-postgres/
│   ├── server-nest-prisma/
│   └── server-dotnet/        # spec-only implementation, proves non-Node backends (P1)
├── bench/                    # reproducible benchmarks vs op-sqlite+custom, WatermelonDB, PowerSync
├── docs/                     # site → GitHub Pages (llms.txt + llms-full.txt generated)
└── skills/sync-flow/SKILL.md # Agent Skill: how to add Sync Flow to an app
```

Tooling: pnpm workspaces, changesets, tsup (TS build), Nitrogen (bindings), CMake (C++). There's no Turborepo; it isn't needed for two packages. GitHub Actions runs lint, typecheck, unit, C++ tests, property tests, Android emulator and iOS simulator e2e (Maestro) and conformance. Releases publish to npm with **provenance through trusted publishing (OIDC)**, so no npm token or 2FA prompt is needed in CI.

## 7. Client engine design (C++)

### 7.1 Threads

| Thread | Does | Never does |
|---|---|---|
| JS thread | Calls the API; renders; runs the first synchronous read of a `useQuery` (reader connection) | Parses sync payloads, applies rows, or waits on the network |
| Writer (1, serial) | All local writes + outbox in one transaction; applies pull pages (batches of 500 rows); rebase | Blocks JS: it returns a Promise |
| Readers (pool of 2–4) | Async query re-runs (WAL allows parallel reads) | Writes |
| Network (native) | URLSession/OkHttp; response bytes are handed to C++ without passing through JS | Calls JS, except `getToken()` |

Rules:

- SQLite runs in WAL mode with `synchronous=NORMAL`, a prepared statement cache and `foreign_keys=OFF` (the server enforces relations).
- In dev builds, a synchronous first read that takes more than 16 ms logs `SF_SLOW_SYNC_READ`, naming the query and suggesting an index.

### 7.2 Local storage layout (in the same encrypted file)

| Table | Purpose |
|---|---|
| `<user tables>` | Exactly the schema columns + hidden `_v INTEGER` (last known server version) and `_sf_state INTEGER` (0 synced, 1 pending, 2 failed). This is the optimistic current state that queries read |
| `_sf_outbox` | `seq INTEGER PK`, `id TEXT` (UUIDv7), `tbl`, `row_id`, `op`, `patch JSON`, `base_version`, `attempts`, `next_at`, `status` (queued/inflight/failed), `error` |
| `_sf_meta` | `cursor`, `schema_version`, `user_id`, `client_id`, `last_pull_at`, `last_push_at` |

- **Write:** one transaction updates the user table and inserts or coalesces the outbox entry.
  - Coalescing is allowed only while the entry is `queued`, not `inflight`:
    - `insert` + `update` → `insert` with the merged fields
    - `insert` + `delete` → both removed
    - `update` + `update` → merged patch
    - `update` + `delete` → `delete`
- **Pull apply:** each page is one transaction. For each row: if the row has no pending outbox entries, write it. Otherwise write the server row, then re-apply the pending patches on top (rebase). Deletes remove the row unless an insert is pending for it. The cursor is saved in the same transaction.
- **Push result:**
  - `applied` / `duplicate` → delete the outbox entry and store the returned `row._v`. The pending flag clears once no other entries remain for the row **and** the pulled cursor is at or past the push `cursor` (write checkpoint).
  - `rejected` → roll back as described in §5.4 and emit an `onError` event.

### 7.3 Sync loop

```
trigger → push (until outbox empty or offline) → pull (until hasMore=false) → idle
```

- **Triggers:**
  - App start and foreground.
  - Network regained (native reachability).
  - A local write (debounced 500 ms).
  - A foreground interval (default 30 s).
  - `db.sync()`.
  - Background task (P1).
  - Server "poke" over SSE (P1).
- **Retry:** exponential backoff of 1 s × 2ⁿ with ±20% jitter, capped at 5 min. Network errors and 5xx retry; 4xx per-mutation results never retry.
- **Quarantine:** after 8 attempts, an entry becomes `failed`.
  - Later entries **for the same row** wait.
  - Other rows continue.
  - `useSyncStatus().failed` lists them, and `db.outbox.retry(id)` or `db.outbox.discard(id)` resolve them.
- Push happens before pull in each cycle, so the device's own writes are visible in the next pull.

### 7.4 Auth

- `getToken()` is called by native code through a Nitro callback, and the result is cached until a 401 or the JWT `exp` is reached.
- On a 401: refresh once, retry once; if it fails again, set `state: 'unauthorized'`, pause sync and **keep the outbox**.
- When `getToken` returns null, sync stops (signed out).

### 7.5 Change notifications

- `sqlite3_update_hook` collects the tables changed in a transaction, and the commit emits one event to JS.
- Each `useQuery` knows which tables it reads: the query builder knows them, and for raw SQL `sqlite3_set_authorizer` records them at prepare time.
- Only queries that read a changed table re-run, asynchronously on the reader pool. React state updates only if the result changed, compared with a cheap hash.

## 8. Security

| Area | Control |
|---|---|
| At rest | SQLite3MultipleCiphers (MIT, built-in crypto, so no OpenSSL to bundle), AES-256 with the SQLCipher-v4-compatible scheme. On by default; `encryption: false` exists for debugging only |
| Key | 256-bit, generated on device. iOS Keychain `AfterFirstUnlockThisDeviceOnly`, so background sync can open it. Android: wrapped with an Android Keystore AES key |
| Per user | File `sf_<sha256(userId)[0:16]>.db` with its own key; `signOut()` closes it and deletes the file, WAL and key |
| Read access | Server `scope` per table; the client filter is never trusted |
| Write access | `scope` is checked again on push; `validate` hooks; the user's existing permissions apply |
| Transport | TLS only (ATS on iOS, network security config on Android); optional certificate pinning (P1) |
| Logs | No row data in production logs; `debug: true` is dev-only and refuses to run in release builds unless `allowInRelease: true` |

## 9. Making it agent-proof (MUST)

The library must be something an AI coding agent can add to an app on the first try. These rules are part of the API design, not just the docs.

1. **One way to do each thing.**
   - No aliases and no overloads that change behaviour.
   - No optional "advanced mode" options in 0.x.
   - If a second way is ever needed, the old one is removed in the next minor release with a codemod.
2. **Types do the teaching.**
   - Table names, columns, filter operators and insert payloads are all inferred from the schema.
   - A typo is a compile error, not a runtime surprise.
3. **No codegen step and no Babel plugin.** Fewer steps, fewer failure points.
4. **Every error is actionable.**
   - The `SyncFlowError` class has `code` (e.g. `SF_EXPO_GO`, `SF_NO_NITRO`, `SF_SCHEMA_INVALID`, `SF_PENDING_WRITES`, `SF_RAW_WRITE`), `message`, `fix` (the exact command or code change) and a `docs` URL.
   - The error catalogue lives in `docs/errors.md` and in `llms.txt`.
5. **`AGENTS.md` ships inside the npm package**, next to `README.md`. It contains:
   - The golden path from §4, word for word.
   - Invariants: "never write synced tables with raw SQL", "never generate ids yourself; use the returned id", "never store tokens in the schema".
   - A "common mistakes → fix" table.
   - A version pin: it describes exactly the installed version.
6. **`llms.txt` + `llms-full.txt`** are generated from `docs/` on every release, hosted on GitHub Pages and included in the package.
7. **CLI for agents.** All commands are non-interactive when flags are given and support `--json`:
   - `npx sync-flow init --app expo --server express --orm prisma --yes` writes the schema file, client file, provider wiring and server route.
   - `npx sync-flow doctor --json` checks the New Architecture, the Nitro install, prebuild state, plugin registration, a server reachable at `url`, and the server spec version.
   - `npx sync-flow test --url ... --token ...` runs the conformance suite with CI exit codes.
8. **Docs that can't rot.** Every code block in `docs/` and `AGENTS.md` is type-checked in CI, with twoslash-style extraction. The examples run in e2e CI.
9. **An Agent Skill** at `skills/sync-flow/SKILL.md`, in the open Agent Skills format, covering how to add Sync Flow to an existing app step by step.
10. **Stable, explicit versioning.** Strict semver, changesets, and a CHANGELOG with "Migration" sections. The protocol version is independent of the package version.

## 10. Quality bar

**Performance targets.** They are measured in `bench/` on a mid-range Android device (Pixel 6a class) and an iPhone 13. A release fails if one regresses by more than 10%.

| Metric | Target |
|---|---|
| Local insert (row + outbox) | p95 < 2 ms |
| Indexed query returning 50 rows | p95 < 3 ms |
| Initial sync of 100k rows (~20 MB JSON before gzip) | < 8 s to apply; no JS frame > 16 ms during sync |
| Peak extra memory during initial sync | < 60 MB |
| JS bundle cost | < 25 KB min+gz |
| Native size | ≤ 2.5 MB per ABI |

**Tests**

- C++ unit tests (GoogleTest): outbox coalescing, rebase, change tracking, migrations.
- **Property tests:** random sequences of offline writes, pulls, crashes, duplicate deliveries and clock skew across 3 simulated devices must always converge to the server state. The core is built as a Node addon in CI only.
- Conformance suite against the Express, Nest and .NET examples on every PR.
- Device e2e (Maestro): offline create → kill app → relaunch → online → row on server; logout wipes; Expo prebuild from scratch.
- Chaos: dropped connections mid-page, 30 s latency, 5xx bursts, expired tokens.

## 11. Roadmap

Estimates are for one developer working with AI coding agents. Each gate must pass before the next phase starts.

| Phase | Weeks | Deliverables | Gate |
|---|---|---|---|
| 0. Skeleton | 1 | Repo, CI, MIT, Nitro module building on iOS + Android + `expo prebuild`; SQLite3MC compiles and opens an encrypted DB; `spec/protocol.md` from §5 | An empty Expo app opens an encrypted DB from JS |
| 1. Local core | 2–4 | Schema + local migrations, CRUD, outbox + coalescing, query builder, `useQuery`/`useRow` with change tracking, per-user files, `signOut` | **A:** a local-only todo app works offline; local perf targets met |
| 2. Sync | 5–7 | Native HTTP, push/pull/rebase, idempotency, quarantine, write checkpoint, auth handling; server kit: handler + Express + Postgres/Prisma/Drizzle adapters + `migrate`; conformance CLI | **B:** property tests converge; kill-mid-sync e2e passes; conformance green |
| 3. Agent-proof 0.1.0 | 8–9 | `init`, `doctor`, error catalogue, `AGENTS.md`, `llms.txt`, docs site, Agent Skill, `expo-todo` example, first npm publish (provenance) | **C:** a fresh agent session, given only the npm package, adds sync to a blank Expo app and passes `doctor` and the e2e without help |
| 3b. Standalone server 0.2.0 | 10–11 | `start` command, YAML config + JSON Schema, JWKS/secret auth, `webhook` + `direct` write modes, LISTEN/NOTIFY → SSE poke, `/health` + `/metrics`, Docker image on GHCR; **pilot app** `examples/pilot-fieldnotes` (Expo + standalone server + Postgres in docker compose) and a .NET webhook example | **D:** the pilot app syncs through the standalone server with zero sync code in the backend; conformance green against the standalone server |
| 4. 0.5 | 12–16 | `version` conflicts + `onConflict`, background sync, Fastify/Nest/fetch adapters, MySQL (library + standalone), Knex, TTL/eviction, dev panel, `/testing` kits, cert pinning | Pilot app in daily use for 2 weeks with no data-loss bugs |
| 5. 1.0 and later | — | API freeze; web/desktop via WASM build of the core; blobs; CDC (logical replication) source for the standalone server; opt-in CRDT text columns | — |

## 12. Decisions log

| Date | Question | Decision |
|---|---|---|
| 2026-11-20 | Minimum versions | **Expo SDK 55+ / React Native 0.83+, New Architecture only.** Owner approved |
| 2026-11-20 | First server database | **Postgres first** (server side only; the app always uses its own encrypted SQLite). MySQL in phase 4 |
| 2026-11-20 | Pilot app | **A dummy pilot app we build ourselves:** `examples/pilot-fieldnotes`, a field-notes app (notes with photos-as-URLs, checklists, shared lists between 2 users). It is chosen to exercise offline edits, shared rows, conflicts and deletes |
| 2026-11-20 | Old Claude Doc v1 plan | **Not used any more.** This file is the only source of truth |
| 2026-11-20 | All-in-one server for any backend language | **Yes:** the standalone server, §4.6, phase 3b |

### Changes from the v1 plan (2026-11-16)

| v1 | v2 | Why |
|---|---|---|
| 4 npm packages + Docker image | 2 packages | "Don't overload with packages" |
| op-sqlite + NetInfo + expo-secure-store + expo-background-task peers | Own Nitro C++/Swift/Kotlin; 1 peer dep | Fewer installs, full control of threads and perf |
| SQLCipher | SQLite3MultipleCiphers | MIT, no OpenSSL to bundle, smaller binary |
| MessagePack + zstd | gzip JSON + C++ parser | Any backend can implement it; still off the JS thread |
| Separate sync-service product (CDC, buckets, MessagePack) | Standalone server = same server package run as a program (Docker/npx), `webhook`/`direct` writes, `_seq` polling + LISTEN/NOTIFY; CDC later | One codebase for library and standalone modes; any backend language gets sync with no code |
| `updated_at` polling cursor | `_seq` + `xid8` horizon | Fixes the skipped-rows bug (P9) |
| Docs as an afterthought | Agent-proof rules (§9) and gate C | Explicit user requirement |

## 13. Research sources

These are the sources behind the pain points. Many are vendor or agency blogs, so treat them as practitioner opinion.

- [Offline-First 2026: Why Most Developers Fail at Data Synchronization](https://dev.to/dataformathub/offline-first-2026-why-most-developers-fail-at-data-synchronization-g4b)
- [Offline-First React Native 2026: WatermelonDB vs PowerSync vs SQLite](https://www.72technologies.com/blog/offline-first-react-native-database-2026)
- [React Native Offline-First in 2026: WatermelonDB vs RxDB vs PowerSync](https://procedure.tech/blogs/react-native-offline-first/)
- [Offline-First Mobile Architecture in 2026 (WatermelonDB, SQLite, CRDTs)](https://www.rubrich.com/blog/offline-first-mobile-architecture-watermelondb-sqlite-crdts-sync)
- [Building offline-first mobile sync: lessons from the trenches](https://dev.to/omaranajar/building-offline-first-mobile-sync-lessons-from-the-trenches-3m21)
- [How to build offline-first SQLite sync in Expo](https://dev.to/sathish_daggula/how-to-build-offline-first-sqlite-sync-in-expo-1lli)
- [React Native offline queue with SQLite (Expo)](https://dev.to/sathish_daggula/react-native-offline-queue-with-sqlite-expo-4bio)
- [The cascading complexity of offline-first sync: why CRDTs alone aren't enough](https://dev.to/biozal/the-cascading-complexity-of-offline-first-sync-why-crdts-alone-arent-enough-2gf)
- [Atlassian: sync failure handling (Trello)](https://www.atlassian.com/blog/how-we-build/sync-failure-handling?nb=1)
- [Offline-first background sync: conflicts, retries, UX](https://appmaster.io/blog/offline-first-background-sync-conflict-retries-ux)
- [Realm Device Sync deprecation discussion](https://github.com/realm/realm-js/discussions/6884) · [MongoDB deprecation notice](https://www.mongodb.com/docs/atlas/device-sdks/deprecation.md) · [Migrate a React Native app to PowerSync](https://mongodb.com/docs/atlas/app-services/sync/migration/reactnativetutorial)
- [op-sqlite: 5x faster, 5x less memory](https://ospfranco.github.io/post/2023/11/09/sqlite-for-react-native,-but-5x-faster-and-5x-less-memory/) · [PowerSync RN database benchmarks](https://github.com/powersync-ja/react-native-database-benchmarks)
- [react-native-mmkv README (V4 is a Nitro Module)](https://raw.githubusercontent.com/mrousavy/react-native-mmkv/main/README.md) · [Nitro Modules](https://github.com/mrousavy/nitro)
- [TanStack Query offline persistence](https://www.mintlify.com/TanStack/query/guides/persistence) · [TanStack DB offline transactions](https://tanstack.com/db/latest/docs/guides/offline-transactions)
- [ElectricSQL vs PowerSync (PowerSync blog)](https://powersync.com/blog/electricsql-electric-next-vs-powersync) · [Comparing local-first frameworks (Neon)](https://neon.tech/blog/comparing-local-first-frameworks-and-approaches) · [Local-first sync engines 2026](https://www.pistack.xyz/posts/2026-10-21-local-first-sync-engines-electricsql-powersync-triplit/)
- [Expo SDK 55 (Legacy Architecture dropped)](https://expo.dev/changelog/sdk-55) · [Expo SDK 57](https://expo.dev/changelog/sdk-57)
- [AGENTS.md: what to include](https://mintlify.com/agentsmd/agents.md/guides/what-to-include) · [AGENTS.md vs CLAUDE.md](https://www.mintlify.com/library/agents-md-vs-claude-md) · [Lynx: AGENTS.md + llms.txt](https://lynxjs.org/ai/agentsmd)

**To verify before phase 0:**

- The current PowerSync and Zero licence terms.
- An Expo SDK newer than 57, if one exists.
- That SQLite3MultipleCiphers builds cleanly with Android NDK r27+ and Xcode 17.
