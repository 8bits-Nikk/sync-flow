import type { HybridObject } from 'react-native-nitro-modules';

/**
 * Platform services implemented in Swift and Kotlin (D-05). Internal.
 *
 * Keys are 256-bit, random, generated on the device and never leave secure storage unwrapped
 * except to open the database (D-07):
 * - iOS: Keychain, kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly (background sync can open it).
 * - Android: wrapped with an AES-256-GCM key in the Android Keystore; wrapped bytes in
 *   SharedPreferences.
 */
export interface SyncFlowPlatform extends HybridObject<{ ios: 'swift'; android: 'kotlin' }> {
  /** Absolute directory for database files; created if missing and excluded from backups. */
  readonly databaseDirectory: string;
  /** Returns the 32-byte key stored under `alias`, generating and storing one first if needed. */
  getOrCreateKey(alias: string): ArrayBuffer;
  /** Deletes the key stored under `alias`. No error if there is none. */
  deleteKey(alias: string): void;
}
