cmake_minimum_required(VERSION 3.10)

project(grid)

# header files
include_directories(include)

# source files
file(GLOB SOURCES "src/*.cpp")

# compile with threading support
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

IF (WIN32)
    # SFML files should be in same directory as CMakesLists.txt
    SET(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SFML-2.5.1/lib/cmake/SFML")

    # static link so that dlls dont need to be with the .exe
    SET(SFML_STATIC_LIBRARIES TRUE)

	SET(HIDE_WINDOWS_CONSOLE TRUE)

	if (HIDE_WINDOWS_CONSOLE)
		SET(GUI_TYPE "WIN32")
		SET(SFML_MAIN "sfml-main")
		SET(MAIN "main")
	ENDIF()
ENDIF()

add_executable(grid ${GUI_TYPE} ${SOURCES})
find_package(SFML 2.5 COMPONENTS system window graphics ${MAIN} REQUIRED)
target_link_libraries(grid sfml-graphics ${SFML_MAIN})


