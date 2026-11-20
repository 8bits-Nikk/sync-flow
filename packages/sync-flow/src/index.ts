/** Package version, kept in sync with package.json by the release workflow. */
export const VERSION = '0.0.0';

export { SyncFlowError } from './errors';
export type { SyncFlowErrorCode } from './errors';

// M0 only: the raw core, for the gate check in examples/expo-todo. Removed from the public API in M1.
export { getCore as unstable_getCore } from './native';
export type { SyncFlowCore as unstable_SyncFlowCore } from './native';
