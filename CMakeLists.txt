cmake_minimum_required(VERSION 3.30)
project(seepeepee LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    sqlite_cmake
    GIT_REPOSITORY  https://github.com/ndsev/sqlite-cmake.git
    GIT_TAG         main
)

FetchContent_MakeAvailable(sqlite_cmake)

add_sqlite(BACKEND PUBLIC VERSION 3.46.0)

file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp")

find_package(ICU REQUIRED COMPONENTS uc)

add_executable(${PROJECT_NAME} ${SOURCES})
add_subdirectory(external/pugixml)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        include
        external/pugixml/src
        external/json-3.12.0/include
        external/cpp-httplib
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        pugixml
        ICU::uc
        SQLite::SQLite3
)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)
