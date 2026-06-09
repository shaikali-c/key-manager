cmake_minimum_required(VERSION 4.0)

project(Wallet)

add_executable(${PROJECT_NAME}
    src/main.cpp src/wallet.cpp
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(unofficial-sodium CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    unofficial-sodium::sodium
    cpr::cpr
    nlohmann_json::nlohmann_json
)

target_include_directories(${PROJECT_NAME} PRIVATE include)
