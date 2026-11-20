/**
 * Every error the package throws (D-27). `fix` says exactly what to do; `docs` links the catalogue.
 * M0 has the first few codes; the full catalogue and CI check arrive in M3-01.
 */
export type SyncFlowErrorCode = 'SF_EXPO_GO' | 'SF_NO_NATIVE';

const FIXES: Record<SyncFlowErrorCode, { message: string; fix: string }> = {
  SF_EXPO_GO: {
    message: 'Sync Flow uses native code, which Expo Go cannot load.',
    fix: 'Use a development build: run `npx expo prebuild` then `npx expo run:android` or `npx expo run:ios`.',
  },
  SF_NO_NATIVE: {
    message: 'The Sync Flow native module is not linked into this app binary.',
    fix: 'Install react-native-nitro-modules, then rebuild the app: `npx expo prebuild --clean` (Expo) or `cd ios && pod install` (bare React Native).',
  },
};

export class SyncFlowError extends Error {
  readonly code: SyncFlowErrorCode;
  readonly fix: string;
  readonly docs: string;

  constructor(code: SyncFlowErrorCode, options?: { cause?: unknown }) {
    const entry = FIXES[code];
    super(`${code}: ${entry.message} Fix: ${entry.fix}`, options);
    this.name = 'SyncFlowError';
    this.code = code;
    this.fix = entry.fix;
    this.docs = `https://github.com/8bits-Nikk/sync-flow/blob/main/docs/errors.md#${code.toLowerCase()}`;
  }
}
