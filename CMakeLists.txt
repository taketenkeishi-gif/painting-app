cmake_minimum_required(VERSION 3.21)

project(LayeredPaintApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PAINT_BUILD_QT_PLATFORM "Build Qt platform conversion layer" ON)
option(PAINT_BUILD_APP "Build Qt desktop application" ON)
option(PAINT_BUILD_TESTS "Build core tests" OFF)

# ── Skia backend (Phase 0-1) ──────────────────────────────────────────────
# Set PAINT_USE_SKIA=ON to build the Skia rendering adapter.
# Requires vcpkg toolchain:
#   cmake -DPAINT_USE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ...
option(PAINT_USE_SKIA "Enable Skia rendering backend (requires vcpkg)" OFF)

# ── ONNX Runtime (SAM2 AI セグメンテーション) ────────────────────────────
# Set PAINT_USE_ONNX=ON to enable local SAM2 inference.
# Requires vcpkg toolchain:
#   cmake -DPAINT_USE_ONNX=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ...
# Or set ONNXRUNTIME_ROOT to the manually downloaded ONNX Runtime directory:
#   cmake -DPAINT_USE_ONNX=ON -DONNXRUNTIME_ROOT=C:/onnxruntime ...
option(PAINT_USE_ONNX "Enable ONNX Runtime for local SAM2 AI selection (requires vcpkg or manual download)" OFF)

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
  list(APPEND _qt_components Core Widgets Network)
endif()
if(PAINT_BUILD_TESTS)
  list(APPEND _qt_components Core)
endif()
if(_qt_components)
  list(REMOVE_DUPLICATES _qt_components)
  find_package(Qt6 REQUIRED COMPONENTS ${_qt_components})
endif()

if(PAINT_USE_SKIA)
  find_package(unofficial-skia CONFIG REQUIRED)
  message(STATUS "Skia backend enabled")
endif()

if(PAINT_USE_ONNX)
  if(DEFINED ONNXRUNTIME_ROOT AND NOT ONNXRUNTIME_ROOT STREQUAL "")
    # 手動ダウンロード版: ONNXRUNTIME_ROOT を指定した場合
    set(ORT_INCLUDE "${ONNXRUNTIME_ROOT}/include")
    set(ORT_LIB    "${ONNXRUNTIME_ROOT}/lib/onnxruntime.lib")
    if(NOT EXISTS "${ORT_INCLUDE}/onnxruntime_cxx_api.h")
      message(FATAL_ERROR "ONNXRUNTIME_ROOT が不正: ${ORT_INCLUDE}/onnxruntime_cxx_api.h が見つかりません")
    endif()
    add_library(onnxruntime_manual INTERFACE)
    target_include_directories(onnxruntime_manual INTERFACE "${ORT_INCLUDE}")
    target_link_libraries(onnxruntime_manual INTERFACE "${ORT_LIB}")
    add_library(onnxruntime::onnxruntime ALIAS onnxruntime_manual)
    message(STATUS "ONNX Runtime (manual): ${ONNXRUNTIME_ROOT}")
  else()
    # vcpkg 版
    find_package(onnxruntime CONFIG REQUIRED)
    message(STATUS "ONNX Runtime (vcpkg): enabled")
  endif()
endif()

# ── Qt Advanced Docking System — CSP-style document tab/float management ──────
# Provides per-document tab detach, float, and re-dock without touching Tool docks.
if(PAINT_BUILD_APP)
  include(FetchContent)
  FetchContent_Declare(
    qtads
    GIT_REPOSITORY https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git
    GIT_TAG        4.3.1
    GIT_SHALLOW    TRUE
  )
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(qtads)
endif()

add_subdirectory(src)

if(PAINT_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
