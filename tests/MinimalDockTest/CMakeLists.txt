cmake_minimum_required(VERSION 3.21)
project(MinimalDockTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_add_executable(MinimalDockTest main.cpp)
target_link_libraries(MinimalDockTest PRIVATE Qt6::Core Qt6::Widgets)
set_target_properties(MinimalDockTest PROPERTIES WIN32_EXECUTABLE TRUE)
