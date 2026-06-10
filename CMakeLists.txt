cmake_minimum_required(VERSION 3.30)

project(Wallet)

add_executable(${PROJECT_NAME}
    src/main.cpp src/wallet.cpp
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_TOOLCHAIN_FILE C:/vcpkg/scripts/buildsystems/vcpkg.cmake)

find_package(unofficial-sodium CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(leveldb CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    unofficial-sodium::sodium
    leveldb::leveldb
    cpr::cpr
    nlohmann_json::nlohmann_json
)

target_include_directories(${PROJECT_NAME} PRIVATE include)
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