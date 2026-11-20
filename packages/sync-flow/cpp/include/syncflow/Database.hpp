#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct sqlite3;

namespace syncflow {

using Key = std::array<std::uint8_t, 32>;  // 256-bit raw key (D-06, D-07)

// One encrypted SQLite connection. Not thread-safe: the engine gives each thread its own
// connection (one writer, a pool of readers, D-20).
class Database {
 public:
  // Opens or creates `path` encrypted with `key` (AES-256, SQLCipher v4 compatible scheme) and
  // applies the standard pragmas: WAL, synchronous=NORMAL, foreign_keys=OFF.
  // Throws Error "SF_DB_OPEN" if the file can't be opened, "SF_DB_KEY" if the key is wrong.
  static std::unique_ptr<Database> open(const std::string& path, const Key& key);

  ~Database();
  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  // Runs one or more statements without results. Throws Error "SF_DB_SQL".
  void exec(const std::string& sql);

  // Runs one statement and returns every row as text (NULL becomes ""). M0 helper for tests and
  // the gate check; the typed statement API replaces it in M1.
  std::vector<std::vector<std::string>> queryText(const std::string& sql);

  // "SQLite3 Multiple Ciphers x.y.z (SQLite a.b.c)".
  static std::string libraryVersion();

 private:
  explicit Database(sqlite3* db) : db_(db) {}
  sqlite3* db_;
};

}  // namespace syncflow
