cmake_minimum_required(VERSION 3.4)
project(ttyhlauncher VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(GNUInstallDirs)
find_package(Git)

option(USE_GIT_VERSION "Get current commit hash from Git if possible" ON)

find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5LinguistTools REQUIRED)

find_package(QuaZip5 REQUIRED)

if(USE_GIT_VERSION AND GIT_FOUND)
  execute_process(COMMAND ${GIT_EXECUTABLE} "rev-parse" "--short" "HEAD"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE _COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set(PROJECT_VERSION "${PROJECT_VERSION}~${_COMMIT_HASH}")
  unset(_COMMIT_HASH)
endif()

configure_file("config.h.in" "config.h" @ONLY)
set(CONFIG_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(ui)
add_subdirectory(resources)
add_subdirectory(translations)
