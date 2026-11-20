cmake_minimum_required(VERSION 3.22)
project(SyncFlow)

set(PACKAGE_NAME SyncFlow)
set(CMAKE_CXX_STANDARD 20)

add_compile_options(-DRN_SERIALIZABLE_STATE=1)

# The portable core (::syncflow) plus vendored SQLite3MultipleCiphers and yyjson. Same CMake file
# the host tests use, so the SQLite compile options can't drift (D-06).
set(SYNCFLOW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(../cpp ${CMAKE_BINARY_DIR}/syncflow_core)

add_library(${PACKAGE_NAME} SHARED
  src/main/cpp/cpp-adapter.cpp
  ../cpp/HybridSyncFlowCore.cpp
)

include(${CMAKE_SOURCE_DIR}/../nitrogen/generated/android/SyncFlow+autolinking.cmake)

include_directories("src/main/cpp" "../cpp")

find_library(LOG_LIB log)
target_link_libraries(${PACKAGE_NAME} syncflow_core ${LOG_LIB} android)
