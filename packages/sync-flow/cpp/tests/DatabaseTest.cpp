#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sqlite3mc_amalgamation.h>

#include "syncflow/Database.hpp"
#include "syncflow/Error.hpp"

namespace fs = std::filesystem;
using syncflow::Database;
using syncflow::Error;
using syncflow::Key;

namespace {

Key makeKey(std::uint8_t seed) {
  Key k{};
  for (std::size_t i = 0; i < k.size(); ++i) k[i] = static_cast<std::uint8_t>(seed + i * 7);
  return k;
}

class DatabaseTest : public ::testing::Test {
 protected:
  void SetUp() override {
    dir_ = fs::temp_directory_path() /
           ("sf_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + "_" +
            ::testing::UnitTest::GetInstance()->current_test_info()->name());
    fs::remove_all(dir_);
    fs::create_directories(dir_);
    path_ = (dir_ / "test.db").string();
  }
  void TearDown() override { fs::remove_all(dir_); }

  fs::path dir_;
  std::string path_;
};

TEST_F(DatabaseTest, WritesAndReadsBackAfterReopen) {
  {
    auto db = Database::open(path_, makeKey(1));
    db->exec("CREATE TABLE todos(id TEXT PRIMARY KEY, title TEXT); "
             "INSERT INTO todos VALUES('a', 'Milk');");
  }
  auto db = Database::open(path_, makeKey(1));
  auto rows = db->queryText("SELECT id, title FROM todos");
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows[0][0], "a");
  EXPECT_EQ(rows[0][1], "Milk");
}

TEST_F(DatabaseTest, AppliesStandardPragmas) {
  auto db = Database::open(path_, makeKey(1));
  EXPECT_EQ(db->queryText("PRAGMA journal_mode")[0][0], "wal");
  EXPECT_EQ(db->queryText("PRAGMA synchronous")[0][0], "1");  // NORMAL
  EXPECT_EQ(db->queryText("PRAGMA foreign_keys")[0][0], "0");
}

TEST_F(DatabaseTest, WrongKeyIsRejected) {
  { Database::open(path_, makeKey(1))->exec("CREATE TABLE t(x);"); }
  try {
    Database::open(path_, makeKey(2));
    FAIL() << "expected SF_DB_KEY";
  } catch (const Error& e) {
    EXPECT_EQ(e.code(), "SF_DB_KEY");
  }
}

TEST_F(DatabaseTest, FileIsEncryptedOnDisk) {
  {
    auto db = Database::open(path_, makeKey(1));
    db->exec("CREATE TABLE t(secret TEXT); INSERT INTO t VALUES('hunter2-plaintext');"
             "PRAGMA wal_checkpoint(TRUNCATE);");
  }
  std::ifstream in(path_, std::ios::binary);
  std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  ASSERT_GT(bytes.size(), 16u);
  EXPECT_NE(bytes.substr(0, 15), "SQLite format 3");
  EXPECT_EQ(bytes.find("hunter2-plaintext"), std::string::npos);
}

TEST_F(DatabaseTest, PlainSqliteCannotReadTheFile) {
  { Database::open(path_, makeKey(1))->exec("CREATE TABLE t(x);"); }
  sqlite3* raw = nullptr;
  ASSERT_EQ(sqlite3_open_v2(path_.c_str(), &raw, SQLITE_OPEN_READONLY, nullptr), SQLITE_OK);
  EXPECT_NE(sqlite3_exec(raw, "SELECT count(*) FROM sqlite_schema", nullptr, nullptr, nullptr),
            SQLITE_OK);
  sqlite3_close_v2(raw);
}

TEST_F(DatabaseTest, ReportsLibraryVersion) {
  EXPECT_NE(Database::libraryVersion().find("SQLite3 Multiple Ciphers 2.7.1"), std::string::npos);
}

TEST_F(DatabaseTest, BadPathThrowsOpenError) {
  try {
    Database::open((dir_ / "missing" / "x.db").string(), makeKey(1));
    FAIL() << "expected SF_DB_OPEN";
  } catch (const Error& e) {
    EXPECT_EQ(e.code(), "SF_DB_OPEN");
  }
}

}  // namespace
