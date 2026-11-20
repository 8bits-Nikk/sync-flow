import { NitroModules } from 'react-native-nitro-modules';
import type { SyncFlowCore } from './specs/SyncFlowCore.nitro';

let core: SyncFlowCore | undefined;

/** Lazily creates the native core so importing the package never touches native code. */
export function getCore(): SyncFlowCore {
  core ??= NitroModules.createHybridObject<SyncFlowCore>('SyncFlowCore');
  return core;
}

export type { SyncFlowCore };
