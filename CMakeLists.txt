cmake_minimum_required(VERSION 3.14)

project(
    svclinker
    VERSION 0.1
    DESCRIPTION "Linker service of Remote Run-time Linker project."
    LANGUAGES CXX
)

include("${CMAKE_SOURCE_DIR}/../cmake/find_arch.cmake")

file(GLOB SOURCE_FILES
    "*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_link_directories(
    ${PROJECT_NAME}
    PRIVATE "../librlcom/build/${ARCH}/$<CONFIG>"
    PRIVATE "../librlcrypto/build/${ARCH}/$<CONFIG>"
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE stdc++fs librlcom librlcrypto
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE "${PROJECT_SOURCE_DIR}/vendor/wine"
    PRIVATE "${PROJECT_SOURCE_DIR}/.."
)
