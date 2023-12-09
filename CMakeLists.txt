cmake_minimum_required(VERSION 3.14)

project(
    svcrrlinker
    VERSION 0.1
    DESCRIPTION "Linker service of Remote Run-time Linker project."
    LANGUAGES CXX
)

set(SOURCE_FILES
    "service/coff.cpp"
    "service/librarian.cpp"
    "service/posix_connection.cpp"
    "service/unix_connection.cpp"
    "service/library.cpp"
    "service/main.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC include "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC stdc++fs librrlcom librrlcrypto
)
