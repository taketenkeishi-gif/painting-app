set(CORE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/core/buffer/PixelBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/document/Document.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/layer/Layer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/render/Renderer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/selection/SelectionMask.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/selection/SelectionEngine.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/ToolType.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/ToolManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/BrushTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/EraserTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/EyedropperTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/HandTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/ZoomTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/LineTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/RectSelectionTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/FillTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/MoveLayerTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/AiSelectTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/GradientTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/ai/GenerativeFillEngine.cpp
)

add_library(paint_core STATIC ${CORE_SOURCES})
set_target_properties(paint_core PROPERTIES
  AUTOMOC ON
)
target_include_directories(paint_core PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(paint_core PUBLIC Qt6::Core)
if(MSVC)
  target_compile_options(paint_core PUBLIC /utf-8)
endif()

if(PAINT_BUILD_QT_PLATFORM)
  add_library(paint_qt_platform STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/qt/QtImageConverter.cpp
  )
  set_target_properties(paint_qt_platform PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
  )
  target_include_directories(paint_qt_platform PUBLIC ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(paint_qt_platform PUBLIC paint_core Qt6::Gui)
endif()

# ── Skia backend adapter (Phase 0-1) ─────────────────────────────────────
if(PAINT_USE_SKIA)
  add_library(paint_skia_platform STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/skia/SkiaPixelBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/skia/SkiaRenderer.cpp
  )
  target_compile_definitions(paint_skia_platform PUBLIC PAINT_USE_SKIA)
  target_include_directories(paint_skia_platform PUBLIC ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(paint_skia_platform PUBLIC paint_core unofficial::skia::skia)
  if(MSVC)
    target_compile_options(paint_skia_platform PRIVATE /utf-8)
  endif()
endif()

set(APP_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/AppController.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/ui/ToolDescriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/ui/UiState.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/ui/IconLoader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/mainwindow/MainWindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/canvasview/CanvasWidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/AiPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/LayerPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/ToolPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/SubToolPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/ToolPropertyPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/ColorWheelWidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/GenerativeFillDialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/ComfyUiClient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/resources/icons.qrc
)


if(PAINT_BUILD_APP)
  if(WIN32)
    list(APPEND APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/app/resources/app.rc)
  endif()
  add_executable(paint_app ${APP_SOURCES})
  set_target_properties(paint_app PROPERTIES
    OUTPUT_NAME "LayeredPaintApp"
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
  )
  target_include_directories(paint_app PRIVATE ${PROJECT_SOURCE_DIR}/src)
  if(MSVC)
    target_compile_options(paint_app PRIVATE /utf-8)
  endif()
  target_link_libraries(paint_app
    PRIVATE
      paint_core
      paint_qt_platform
      Qt6::Core
      Qt6::Gui
      Qt6::Widgets
      Qt6::Network
  )
  if(PAINT_USE_SKIA)
    target_link_libraries(paint_app PRIVATE paint_skia_platform)
    target_compile_definitions(paint_app PRIVATE PAINT_USE_SKIA)
  endif()
endif()
