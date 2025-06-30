cmake_minimum_required(VERSION 3.10)
project(client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

find_package(Boost REQUIRED COMPONENTS system)

#add_executable(lab4 main.cpp encryptor.cpp dir_checker.cpp cipher.cpp)

add_executable(client
    #test_main.cpp
    client.cpp
    encryption.cpp
    cipher.cpp
    unique_words.cpp
    token_gen.cpp
    ../common/base64.cpp
)
target_link_libraries(client PRIVATE ${OPENSSL_LIBRARIES} Boost::system)

# add_executable(server server.cpp)
# target_link_libraries(server PRIVATE Boost::system pthread)
