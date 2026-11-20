# Sync Flow C++ core. Built three ways:
#   - host (Linux/macOS) for unit and property tests: `pnpm test:cpp` from the repo root
#   - Android, included from android/CMakeLists.txt
#   - iOS, through the podspec (keep SYNCFLOW_SQLITE_DEFINES in sync with it)
cmake_minimum_required(VERSION 3.22)
project(syncflow_core C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# SQLite compile options. One list, used by every build (D-06).
set(SYNCFLOW_SQLITE_DEFINES
  SQLITE_THREADSAFE=2              # multi-thread: each connection is used by one thread at a time
  SQLITE_DQS=0
  SQLITE_DEFAULT_MEMSTATUS=0
  SQLITE_DEFAULT_WAL_SYNCHRONOUS=1 # NORMAL
  SQLITE_LIKE_DOESNT_MATCH_BLOBS
  SQLITE_OMIT_DEPRECATED
  SQLITE_OMIT_LOAD_EXTENSION
  SQLITE_OMIT_SHARED_CACHE
  SQLITE_USE_ALLOCA
  SQLITE_ENABLE_UPDATE_DELETE_LIMIT=0
  SQLITE_TEMP_STORE=2
  HAVE_CIPHER_SQLCIPHER=1
  CODEC_TYPE=CODEC_TYPE_SQLCIPHER
)

add_library(sf_sqlite3mc STATIC third_party/sqlite3mc/sqlite3mc_amalgamation.c)
target_include_directories(sf_sqlite3mc PUBLIC third_party/sqlite3mc)
target_compile_definitions(sf_sqlite3mc PUBLIC ${SYNCFLOW_SQLITE_DEFINES})
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(sf_sqlite3mc PRIVATE -w)   # vendored code: don't drown our warnings
endif()

add_library(sf_yyjson STATIC third_party/yyjson/yyjson.c)
target_include_directories(sf_yyjson PUBLIC third_party/yyjson)

add_library(syncflow_core STATIC
  src/Database.cpp
)
target_include_directories(syncflow_core PUBLIC include)
target_link_libraries(syncflow_core PUBLIC sf_sqlite3mc sf_yyjson)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(syncflow_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

option(SYNCFLOW_BUILD_TESTS "Build the C++ unit tests" ${PROJECT_IS_TOP_LEVEL})
if(SYNCFLOW_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  find_package(Threads REQUIRED)

  enable_testing()
  add_executable(syncflow_tests tests/DatabaseTest.cpp)
  target_link_libraries(syncflow_tests PRIVATE syncflow_core GTest::gtest_main Threads::Threads)
  include(GoogleTest)
  gtest_discover_tests(syncflow_tests)
endif()
