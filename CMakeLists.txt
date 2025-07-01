cmake_minimum_required(VERSION 3.10)
project(json_client_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(OpenSSL REQUIRED)

add_library(base64 STATIC
    common/base64.cpp
    common/base64.h
)

target_include_directories(base64 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

add_library(encryption STATIC
    client/encryption.cpp
    client/encryption.h
    client/cipher.cpp
    client/cipher.h
    client/token_gen.h
    client/token_gen.cpp
    common/sse_types.h
)

target_include_directories(encryption PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

target_link_libraries(encryption
    base64
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::filesystem
)

add_executable(server
    server/server.cpp
)

target_link_libraries(server
    base64
    encryption
    Boost::system
    Boost::filesystem
)

add_executable(client
    client/client.cpp
)

target_link_libraries(client
    base64
    encryption
    Boost::system
    Boost::filesystem
    OpenSSL::SSL
    OpenSSL::Crypto
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
