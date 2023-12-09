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

set(HEADER_FILES
    "include/rrlinker/service/coff.hpp"
    "include/rrlinker/service/librarian.hpp"
    "include/rrlinker/service/library.hpp"
    "include/rrlinker/service/posix_connection.hpp"
    "include/rrlinker/service/unix_connection.hpp"
)

add_executable(${PROJECT_NAME}
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC stdc++fs librrlcom librrlcrypto
)

include(GNUInstallDirs)
install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/rrlinker/service"
)
