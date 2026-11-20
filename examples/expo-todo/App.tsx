import { useEffect, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import { unstable_getCore, unstable_getPlatform, VERSION } from '@nikhilll/sync-flow';

// M0 gate check: open an encrypted database from JS with a key from Keychain/Keystore, write one
// row, read it back. The launch counter proves the stored key reopens the same file next launch.
function runGateCheck(): string {
  const core = unstable_getCore();
  const platform = unstable_getPlatform();
  const key = platform.getOrCreateKey('m0-gate');
  core.open(`${platform.databaseDirectory}/m0-gate.db`, key);
  core.exec(
    'CREATE TABLE IF NOT EXISTS todos(id TEXT PRIMARY KEY, title TEXT);' +
      'CREATE TABLE IF NOT EXISTS launches(n INTEGER);' +
      "INSERT OR REPLACE INTO todos VALUES('m0', 'Milk');" +
      'INSERT INTO launches VALUES(1);',
  );
  const rows = core.queryText("SELECT title FROM todos WHERE id = 'm0'");
  const launches = core.queryText('SELECT count(*) FROM launches');
  core.close();
  return `DB OK: ${rows[0]?.[0] ?? '(missing)'} | launches=${launches[0]?.[0] ?? '?'}`;
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
