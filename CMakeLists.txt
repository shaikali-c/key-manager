cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Wallet)

add_executable(${PROJECT_NAME}
    src/main.cpp src/wallet.cpp
)

find_package(cpr REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libsodium REQUIRED libsodium)
find_package(leveldb REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${libsodium_LIBRARIES}
    leveldb::leveldb
    cpr::cpr
    Threads::Threads
    nlohmann_json::nlohmann_json
)

target_include_directories(${PROJECT_NAME} PRIVATE include ${libsodium_INCLUDE_DIRS}
target_compile_options(myapp PRIVATE ${libsodium_CFLAGS}))
target_compile_features(Wallet PRIVATE cxx_std_20)
target_precompile_headers(
    ${PROJECT_NAME}
    PRIVATE
    <vector>
    <array>
    <memory>
    <iostream>
    <cstring>
    <stdexcept>
    <chrono>
    <utility>
    <cpr/cpr.h>
    <leveldb/db.h>
    <nlohmann/json.hpp>
    <sodium.h>
)
