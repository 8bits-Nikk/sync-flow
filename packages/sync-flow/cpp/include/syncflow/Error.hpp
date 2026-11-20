#pragma once

#include <stdexcept>
#include <string>

namespace syncflow {

// Every error leaving the core carries a stable SF_* code (D-27). The JS layer maps the code to
// a SyncFlowError with `fix` and `docs` from the error catalogue.
class Error : public std::runtime_error {
 public:
  Error(std::string code, const std::string& message)
      : std::runtime_error(code + ": " + message), code_(std::move(code)) {}

  const std::string& code() const noexcept { return code_; }

 private:
  std::string code_;
};

}  // namespace syncflow
