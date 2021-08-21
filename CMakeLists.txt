cmake_minimum_required(VERSION 3.10)

project(Grid)

# Windows specific config
IF (WIN32)
    # Include local sfml cmake config
    set(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SFML-2.5.1/lib/cmake/SFML")
    # Link sfml statically
    set(SFML_STATIC_LIBRARIES TRUE)
ENDIF()

# Find SFML shared libraries
find_package(SFML 2.5.1 COMPONENTS graphics REQUIRED)

include_directories(include)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

file(GLOB SOURCES "src/*.cpp")
add_executable(Grid ${SOURCES})

find_package(SFML 2.5 
  COMPONENTS system window graphics REQUIRED)
target_link_libraries(Grid sfml-graphics)
