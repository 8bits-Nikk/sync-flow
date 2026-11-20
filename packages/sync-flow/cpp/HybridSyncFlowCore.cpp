#include "HybridSyncFlowCore.hpp"

#include <cstring>

#include "syncflow/Error.hpp"

namespace margelo::nitro::syncflow {

std::string HybridSyncFlowCore::getLibraryVersion() {
  return ::syncflow::Database::libraryVersion();
}

void HybridSyncFlowCore::open(const std::string& path, const std::shared_ptr<ArrayBuffer>& key) {
  if (key == nullptr || key->size() != sizeof(::syncflow::Key)) {
    throw ::syncflow::Error("SF_DB_KEY", "key must be exactly 32 bytes");
  }
  ::syncflow::Key k{};
  std::memcpy(k.data(), key->data(), k.size());
  db_.reset();  // close any previous database first
  db_ = ::syncflow::Database::open(path, k);
}

void HybridSyncFlowCore::exec(const std::string& sql) { db().exec(sql); }

std::vector<std::vector<std::string>> HybridSyncFlowCore::queryText(const std::string& sql) {
  return db().queryText(sql);
}

void HybridSyncFlowCore::close() { db_.reset(); }

::syncflow::Database& HybridSyncFlowCore::db() {
  if (db_ == nullptr) {
    throw ::syncflow::Error("SF_DB_CLOSED", "no database is open; call open() first");
  }
  return *db_;
}

}  // namespace margelo::nitro::syncflow
