cmake_minimum_required(VERSION 3.5)

project(svclinker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCE_FILES
    "*.cpp"
)

set(LINK_LIBRARIES stdc++fs librlcom.a)

link_directories(${PROJECT_SOURCE_DIR}/../librlcom/build)

add_library(stdc++fs UNKNOWN IMPORTED)
set_property(TARGET stdc++fs PROPERTY IMPORTED_LOCATION "/usr/lib/libstdc++fs.a")

include_directories("${PROJECT_SOURCE_DIR}/vendor/wine")
include_directories("${PROJECT_SOURCE_DIR}/../librlcom")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} ${LINK_LIBRARIES})

