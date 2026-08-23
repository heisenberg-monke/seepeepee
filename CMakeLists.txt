cmake_minimum_required(VERSION 3.30)
project(seepeepee LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})
add_subdirectory(external/pugixml)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        include
        external/pugixml/src
        external/utfcpp-4.1.1
        external/json-3.12.0/include
        external/cpp-httplib
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        pugixml
)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)
