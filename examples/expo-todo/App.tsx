import { Paths } from 'expo-file-system';
import { useEffect, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import { unstable_getCore, VERSION } from '@nikhilll/sync-flow';

// M0 gate check: open an encrypted database from JS, write one row, read it back.
// The fixed key is for this smoke test only; real keys come from Keychain/Keystore (M0-08, D-07).
const TEST_KEY = new Uint8Array(32).map((_, i) => (i * 7 + 1) & 0xff).buffer;

function runGateCheck(): string {
  const core = unstable_getCore();
  const path = `${decodeURIComponent(new URL(Paths.document.uri).pathname)}/m0-gate.db`;
  core.open(path, TEST_KEY);
  core.exec(
    'CREATE TABLE IF NOT EXISTS todos(id TEXT PRIMARY KEY, title TEXT);' +
      "INSERT OR REPLACE INTO todos VALUES('m0', 'Milk');",
  );
  const rows = core.queryText("SELECT title FROM todos WHERE id = 'm0'");
  const cipher = core.queryText('PRAGMA cipher');
  core.close();
  return `DB OK: ${rows[0]?.[0] ?? '(missing)'} | cipher=${cipher[0]?.[0] ?? '?'}`;
}

export default function App() {
  const [result, setResult] = useState('running…');
  const [version, setVersion] = useState('');

  useEffect(() => {
    try {
      setVersion(unstable_getCore().libraryVersion);
      setResult(runGateCheck());
    } catch (e) {
      setResult(`DB FAILED: ${e instanceof Error ? e.message : String(e)}`);
    }
  }, []);

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Sync Flow {VERSION}</Text>
      <Text testID="sf-version">{version}</Text>
      <Text testID="sf-gate" style={styles.result}>
        {result}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, alignItems: 'center', justifyContent: 'center', padding: 24, gap: 12 },
  title: { fontSize: 20, fontWeight: '600' },
  result: { fontSize: 16 },
});
