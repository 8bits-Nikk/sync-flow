#pragma once

#include <memory>
#include <string>
#include <vector>

#include "HybridSyncFlowCoreSpec.hpp"
#include "syncflow/Database.hpp"

namespace margelo::nitro::syncflow {

// JSI binding for the core (D-04). Thin: all logic lives in ::syncflow.
class HybridSyncFlowCore : public HybridSyncFlowCoreSpec {
 public:
  HybridSyncFlowCore() : HybridObject(TAG) {}

  std::string getLibraryVersion() override;
  void open(const std::string& path, const std::shared_ptr<ArrayBuffer>& key) override;
  void exec(const std::string& sql) override;
  std::vector<std::vector<std::string>> queryText(const std::string& sql) override;
  void close() override;

 private:
  ::syncflow::Database& db();
  std::unique_ptr<::syncflow::Database> db_;
};

}  // namespace margelo::nitro::syncflow
