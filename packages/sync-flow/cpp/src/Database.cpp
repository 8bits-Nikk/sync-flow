#include "syncflow/Database.hpp"

#include <sqlite3mc_amalgamation.h>

#include "syncflow/Error.hpp"

namespace syncflow {
namespace {

std::string sqliteMessage(sqlite3* db) {
  return db != nullptr ? sqlite3_errmsg(db) : "out of memory";
}

// Raw-key form x'<64 hex>' skips the passphrase KDF, so opening costs microseconds, not the
// 256k PBKDF2 rounds SQLCipher v4 uses for passphrases. The key is random, so no KDF is needed.
std::string rawKeyLiteral(const Key& key) {
  static constexpr char hex[] = "0123456789abcdef";
  std::string out = "x'";
  out.reserve(2 + key.size() * 2 + 1);
  for (std::uint8_t b : key) {
    out.push_back(hex[b >> 4]);
    out.push_back(hex[b & 0x0f]);
  }
  out.push_back('\'');
  return out;
}

}  // namespace

std::unique_ptr<Database> Database::open(const std::string& path, const Key& key) {
  sqlite3* raw = nullptr;
  const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX;
  int rc = sqlite3_open_v2(path.c_str(), &raw, flags, nullptr);
  std::unique_ptr<Database> db(new Database(raw));  // closes `raw` on every error path below
  if (rc != SQLITE_OK) {
    throw Error("SF_DB_OPEN", "cannot open " + path + ": " + sqliteMessage(raw));
  }

  // SQLCipher v4 compatible scheme (legacy=4), AES-256 (D-06).
  if (sqlite3mc_config(raw, "cipher", CODEC_TYPE_SQLCIPHER) != CODEC_TYPE_SQLCIPHER ||
      sqlite3mc_config_cipher(raw, "sqlcipher", "legacy", 4) != 4) {
    throw Error("SF_DB_OPEN", "cannot configure the SQLCipher scheme");
  }
  const std::string literal = rawKeyLiteral(key);
  rc = sqlite3_key_v2(raw, "main", literal.data(), static_cast<int>(literal.size()));
  if (rc != SQLITE_OK) {
    throw Error("SF_DB_KEY", "cannot set the key: " + sqliteMessage(raw));
  }

  // The first read verifies the key: a wrong key makes the file look like garbage.
  rc = sqlite3_exec(raw, "SELECT count(*) FROM sqlite_schema", nullptr, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    throw Error("SF_DB_KEY", "wrong key or not a Sync Flow database: " + path);
  }

  db->exec(
      "PRAGMA journal_mode=WAL;"
      "PRAGMA synchronous=NORMAL;"
      "PRAGMA foreign_keys=OFF;");
  return db;
}

Database::~Database() {
  if (db_ != nullptr) {
    sqlite3_close_v2(db_);
  }
}

void Database::exec(const std::string& sql) {
  char* err = nullptr;
  if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
    std::string msg = err != nullptr ? err : sqliteMessage(db_);
    sqlite3_free(err);
    throw Error("SF_DB_SQL", msg);
  }
}

std::vector<std::vector<std::string>> Database::queryText(const std::string& sql) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    throw Error("SF_DB_SQL", sqliteMessage(db_));
  }
  std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> guard(stmt, &sqlite3_finalize);

  std::vector<std::vector<std::string>> rows;
  const int columns = sqlite3_column_count(stmt);
  int rc;
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    auto& row = rows.emplace_back();
    row.reserve(columns);
    for (int i = 0; i < columns; ++i) {
      const auto* text = sqlite3_column_text(stmt, i);
      row.emplace_back(text != nullptr ? reinterpret_cast<const char*>(text) : "");
    }
  }
  if (rc != SQLITE_DONE) {
    throw Error("SF_DB_SQL", sqliteMessage(db_));
  }
  return rows;
}

std::string Database::libraryVersion() {
  return std::string(SQLITE3MC_VERSION_STRING) + " (SQLite " + sqlite3_libversion() + ")";
}

}  // namespace syncflow
