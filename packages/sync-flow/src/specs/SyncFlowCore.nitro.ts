import type { HybridObject } from 'react-native-nitro-modules';

/**
 * The C++ core, exposed through JSI (D-04). Internal: apps use the public API from `src/index.ts`,
 * never this object directly.
 *
 * M0 surface only: enough to prove an encrypted database opens from JS. The real engine API
 * (writer/readers, outbox, queries) replaces `exec`/`queryText` in M1.
 */
export interface SyncFlowCore extends HybridObject<{ ios: 'c++'; android: 'c++' }> {
  /** "SQLite3 Multiple Ciphers x.y.z (SQLite a.b.c)". */
  readonly libraryVersion: string;

  /** Opens (or creates) an encrypted database. `key` must be exactly 32 bytes. */
  open(path: string, key: ArrayBuffer): void;
  /** Runs statements without results. */
  exec(sql: string): void;
  /** Runs one statement; every value as text. */
  queryText(sql: string): string[][];
  /** Closes the database. Safe to call twice. */
  close(): void;
}
