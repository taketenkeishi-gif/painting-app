set(CORE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/core/buffer/PixelBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/document/Document.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/layer/Layer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/render/Renderer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/registry/FeatureRegistry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/registry/RendererRegistry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/registry/ToolRegistry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/selection/SelectionMask.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/ToolType.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/ToolManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/BrushTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/PenTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/EraserTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/EyedropperTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/HandTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/ZoomTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/LineTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/RectSelectionTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/FillTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/MoveLayerTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/FeatureManifest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/common/DescriptorUtils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/brush/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/brush/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/eraser/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/eraser/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/eyedropper/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/eyedropper/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/fill/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/fill/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/hand/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/hand/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/line/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/line/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/move_layer/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/move_layer/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/pen/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/pen/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/rect_selection/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/rect_selection/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/zoom/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/zoom/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/gradient/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/gradient/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/comic/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/comic/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/text/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/text/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/ruler/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/ruler/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/line_correction/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/line_correction/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/operation/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/operation/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/airbrush/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/airbrush/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/color_mix/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/color_mix/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/liquify/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/liquify/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/clone_stamp/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/clone_stamp/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/sketch/Feature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/sketch/Descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/requested_tools/RequestedToolsRuntime.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/object_editing/ObjectModel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/object_editing/ObjectSelectionService.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/object_editing/ObjectTransformController.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/object_editing/ObjectOverlayRenderer.cpp
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
  ${CMAKE_CURRENT_SOURCE_DIR}/app/commands/CommandRegistry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/ui/ToolDescriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/ui/UiState.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/ui/IconLoader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/mainwindow/MainWindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/canvasview/CanvasWidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/LayerPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/ToolPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/SubToolPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/ToolPropertyPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/ColorWheelWidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/features/object_editing/text_editor/TextEditorFeature.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/resources/icons.qrc
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
  if(WIN32)
    target_link_libraries(paint_app PRIVATE imm32)
  endif()
endif()
