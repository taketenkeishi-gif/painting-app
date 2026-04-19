set(CORE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/core/buffer/PixelBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/document/Document.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/layer/Layer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/render/Renderer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/BrushTool.cpp
)

add_library(paint_core STATIC ${CORE_SOURCES})
target_include_directories(paint_core PUBLIC ${PROJECT_SOURCE_DIR}/src)

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

set(APP_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/AppController.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/mainwindow/MainWindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/canvasview/CanvasWidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/LayerPanel.cpp
)

if(PAINT_BUILD_APP)
  add_executable(paint_app ${APP_SOURCES})
  set_target_properties(paint_app PROPERTIES
    OUTPUT_NAME "LayeredPaintApp"
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
  )
  target_include_directories(paint_app PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(paint_app
    PRIVATE
      paint_core
      paint_qt_platform
      Qt6::Core
      Qt6::Gui
      Qt6::Widgets
  )
endif()
