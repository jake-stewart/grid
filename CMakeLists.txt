cmake_minimum_required(VERSION 3.10)

project(Grid)

# header files
include_directories(include)

# source files
file(GLOB SOURCES "src/*.cpp")

# compile with threading support
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# WINDOWS
IF (WIN32)
    # SFML files should be in same directory as CMakesLists.txt
    set(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SFML-2.5.1/lib/cmake/SFML")

    # static link so that dlls dont need to be with the .exe
    set(SFML_STATIC_LIBRARIES TRUE)

    # compiling with WIN32 stops cmd from opening up
    add_executable(Grid WIN32 ${SOURCES})

    # sfml-main stops the need for WinMain() function or whatever
    # now int main() works
    find_package(SFML 2.5 COMPONENTS system window graphics main REQUIRED)
    target_link_libraries(Grid sfml-graphics sfml-main)

# LINUX
ELSE()
    add_executable(Grid ${SOURCES})
    find_package(SFML 2.5 COMPONENTS system window graphics REQUIRED)
    target_link_libraries(Grid sfml-graphics)
ENDIF()


