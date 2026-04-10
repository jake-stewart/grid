cmake_minimum_required(VERSION 3.10)

project(grid)
set(CMAKE_BUILD_TYPE Release)

# header files
include_directories(include)

# source files
file(GLOB SOURCES "src/*.cpp")

# compile with threading support
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# SET(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SFML-3.0.1/lib/cmake/SFML")
#SET(SFML_STATIC_LIBRARIES TRUE)

# Auto-detect Homebrew sfml@2 on macOS
if(APPLE)
    execute_process(
        COMMAND brew --prefix sfml@2
        OUTPUT_VARIABLE BREW_SFML2_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE BREW_SFML2_RESULT
    )
    if(BREW_SFML2_RESULT EQUAL 0 AND EXISTS "${BREW_SFML2_PREFIX}")
        list(INSERT CMAKE_PREFIX_PATH 0 "${BREW_SFML2_PREFIX}")
    endif()
endif()

IF (WIN32)

	SET(HIDE_WINDOWS_CONSOLE TRUE)

	if (HIDE_WINDOWS_CONSOLE)
		SET(GUI_TYPE "WIN32")
		SET(SFML_MAIN "sfml-main")
		SET(MAIN "main")
	ENDIF()
ENDIF()

add_executable(grid ${GUI_TYPE} ${SOURCES})
find_package(SFML 2.6.2 COMPONENTS system window graphics ${MAIN} REQUIRED)
target_link_libraries(grid sfml-graphics ${SFML_MAIN})


