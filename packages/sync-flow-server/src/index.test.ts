import { expect, test } from 'vitest';
import { PROTOCOL_VERSION } from './index';

test('implements protocol v1', () => {
  expect(PROTOCOL_VERSION).toBe(1);
});
