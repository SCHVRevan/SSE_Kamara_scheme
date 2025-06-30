cmake_minimum_required(VERSION 3.10)
project(SSE_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(PQXX_INCLUDE_DIR pqxx/pqxx)
find_library(PQXX_LIBRARY pqxx)

if(NOT PQXX_INCLUDE_DIR OR NOT PQXX_LIBRARY)
    message(FATAL_ERROR "libpqxx not found! Please install libpqxx-dev")
endif()

find_package(Boost 1.66 REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json 3.7 REQUIRED)

add_executable(server
    main.cpp
    server.cpp
    db_connector.cpp
    ../client/encryption.cpp
    ../client/cipher.cpp
    ../common/base64.cpp
)

target_include_directories(server PRIVATE
    ${PQXX_INCLUDE_DIR}
    ../client
)

target_link_libraries(server PRIVATE
    ${PQXX_LIBRARY}
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)
