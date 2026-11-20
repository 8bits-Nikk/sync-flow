require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

# SQLite compile options: keep identical to SYNCFLOW_SQLITE_DEFINES in cpp/CMakeLists.txt (D-06).
sqlite_defines = %w[
  SQLITE_THREADSAFE=2
  SQLITE_DQS=0
  SQLITE_DEFAULT_MEMSTATUS=0
  SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
  SQLITE_LIKE_DOESNT_MATCH_BLOBS
  SQLITE_OMIT_DEPRECATED
  SQLITE_OMIT_LOAD_EXTENSION
  SQLITE_OMIT_SHARED_CACHE
  SQLITE_USE_ALLOCA
  SQLITE_ENABLE_UPDATE_DELETE_LIMIT=0
  SQLITE_TEMP_STORE=2
  HAVE_CIPHER_SQLCIPHER=1
  CODEC_TYPE=CODEC_TYPE_SQLCIPHER
]

Pod::Spec.new do |s|
  s.name         = "SyncFlow"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = "https://github.com/8bits-Nikk/sync-flow"
  s.license      = package["license"]
  s.authors      = "Sync Flow contributors"

  s.platforms    = { :ios => "15.1" }
  s.source       = { :git => "https://github.com/8bits-Nikk/sync-flow.git", :tag => "v#{s.version}" }

  s.source_files = [
    "ios/**/*.{swift,m,mm,h}",
    "cpp/*.{hpp,cpp}",
    "cpp/include/**/*.hpp",
    "cpp/src/**/*.{hpp,cpp}",
    "cpp/third_party/sqlite3mc/sqlite3mc_amalgamation.{c,h}",
    "cpp/third_party/yyjson/yyjson.{c,h}",
  ]
  # Vendored sqlite3.h must never shadow the system header for other pods.
  s.private_header_files = "cpp/third_party/**/*.h"
  s.preserve_paths = "cpp/third_party/**/*"

  s.pod_target_xcconfig = {
    "HEADER_SEARCH_PATHS" => [
      "\"$(PODS_TARGET_SRCROOT)/cpp\"",
      "\"$(PODS_TARGET_SRCROOT)/cpp/include\"",
      "\"$(PODS_TARGET_SRCROOT)/cpp/third_party/sqlite3mc\"",
      "\"$(PODS_TARGET_SRCROOT)/cpp/third_party/yyjson\"",
    ].join(" "),
    "GCC_PREPROCESSOR_DEFINITIONS" => "$(inherited) " + sqlite_defines.join(" "),
    "CLANG_CXX_LANGUAGE_STANDARD" => "c++20",
  }

  load "nitrogen/generated/ios/SyncFlow+autolinking.rb"
  add_nitrogen_files(s)

  s.dependency "React-jsi"
  s.dependency "React-callinvoker"
  install_modules_dependencies(s)
end
