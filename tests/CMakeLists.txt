add_executable(core_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/core/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/DocumentTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/RendererTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/LayerTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/BrushToolTests.cpp
)

target_include_directories(core_tests PRIVATE ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/core)
target_link_libraries(core_tests PRIVATE paint_core)

add_test(NAME core_tests COMMAND core_tests)

add_executable(app_smoke_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/app/AppControllerSmokeTests.cpp
  ${PROJECT_SOURCE_DIR}/src/app/bridge/AppController.cpp
  ${PROJECT_SOURCE_DIR}/src/app/ui/ToolDescriptor.cpp
  ${PROJECT_SOURCE_DIR}/src/app/ui/UiState.cpp
)
set_target_properties(app_smoke_tests PROPERTIES
  AUTOMOC ON
  AUTOUIC ON
  AUTORCC ON
)
target_include_directories(app_smoke_tests PRIVATE ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/core)
target_link_libraries(app_smoke_tests PRIVATE paint_core Qt6::Core)

add_test(NAME app_smoke_tests COMMAND app_smoke_tests)
set_tests_properties(app_smoke_tests PROPERTIES
  ENVIRONMENT "PATH=C:/CraftRoot_KF6/bin;$ENV{PATH}"
)
