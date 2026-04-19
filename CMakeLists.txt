cmake_minimum_required(VERSION 3.21)

project(LayeredPaintApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PAINT_BUILD_QT_PLATFORM "Build Qt platform conversion layer" ON)
option(PAINT_BUILD_APP "Build Qt desktop application" ON)
option(PAINT_BUILD_TESTS "Build core tests" OFF)

if(DEFINED CMAKE_PREFIX_PATH AND NOT CMAKE_PREFIX_PATH STREQUAL "")
  message(STATUS "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
endif()

if(PAINT_BUILD_TESTS AND CMAKE_VERSION VERSION_LESS "4.3.1")
  message(FATAL_ERROR "PAINT_BUILD_TESTS=ON requires CMake >= 4.3.1 in this environment.")
endif()

if(PAINT_BUILD_APP AND NOT PAINT_BUILD_QT_PLATFORM)
  message(FATAL_ERROR "PAINT_BUILD_APP=ON requires PAINT_BUILD_QT_PLATFORM=ON.")
endif()

set(_qt_components)
if(PAINT_BUILD_QT_PLATFORM OR PAINT_BUILD_APP)
  list(APPEND _qt_components Gui)
endif()
if(PAINT_BUILD_APP)
  list(APPEND _qt_components Core Widgets)
endif()
if(PAINT_BUILD_TESTS)
  list(APPEND _qt_components Core)
endif()
if(_qt_components)
  list(REMOVE_DUPLICATES _qt_components)
  find_package(Qt6 REQUIRED COMPONENTS ${_qt_components})
endif()

add_subdirectory(src)

if(PAINT_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
