import { NitroModules } from 'react-native-nitro-modules';
import { SyncFlowError } from './errors';
import type { SyncFlowCore } from './specs/SyncFlowCore.nitro';
import type { SyncFlowPlatform } from './specs/SyncFlowPlatform.nitro';

let core: SyncFlowCore | undefined;
let platform: SyncFlowPlatform | undefined;

function isExpoGo(): boolean {
  const g = globalThis as { expo?: { modules?: Record<string, unknown> } };
  return g.expo?.modules?.ExpoGo !== undefined;
}

function create<T extends SyncFlowCore | SyncFlowPlatform>(name: string): T {
  try {
    return NitroModules.createHybridObject<T>(name);
  } catch (cause) {
    throw new SyncFlowError(isExpoGo() ? 'SF_EXPO_GO' : 'SF_NO_NATIVE', { cause });
  }
}

/** Lazily creates the native core so importing the package never touches native code. */
export function getCore(): SyncFlowCore {
  core ??= create<SyncFlowCore>('SyncFlowCore');
  return core;
}

/** Keychain/Keystore keys and the database directory (Swift/Kotlin). */
export function getPlatform(): SyncFlowPlatform {
  platform ??= create<SyncFlowPlatform>('SyncFlowPlatform');
  return platform;
}

export type { SyncFlowCore, SyncFlowPlatform };
