import { NitroModules } from 'react-native-nitro-modules';
import { SyncFlowError } from './errors';
import type { SyncFlowCore } from './specs/SyncFlowCore.nitro';

let core: SyncFlowCore | undefined;

function isExpoGo(): boolean {
  const g = globalThis as { expo?: { modules?: Record<string, unknown> } };
  return g.expo?.modules?.ExpoGo !== undefined;
}

/** Lazily creates the native core so importing the package never touches native code. */
export function getCore(): SyncFlowCore {
  if (core === undefined) {
    try {
      core = NitroModules.createHybridObject<SyncFlowCore>('SyncFlowCore');
    } catch (cause) {
      throw new SyncFlowError(isExpoGo() ? 'SF_EXPO_GO' : 'SF_NO_NATIVE', { cause });
    }
  }
  return core;
}

export type { SyncFlowCore };
