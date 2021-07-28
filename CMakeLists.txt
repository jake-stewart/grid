cmake_minimum_required(VERSION 3.10)

project(Grid)
include_directories(include)

file(GLOB SOURCES "src/*.cpp")
add_executable(Grid ${SOURCES})

find_package(SFML 2.5 
  COMPONENTS system window graphics REQUIRED)
target_link_libraries(Grid sfml-graphics)

