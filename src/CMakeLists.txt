set(CORE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/core/brush/DabGenerator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/brush/DabRenderer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/brush/StrokeProcessor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/buffer/PixelBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/document/Document.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/layer/Layer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/render/Renderer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/selection/SelectionMask.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/selection/SelectionEngine.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/selection/SelectionRefiner.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/selection/MaskExporter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/selection/providers/ClassicProvider.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/ToolType.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/ToolManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/BrushTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/EraserTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/EyedropperTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/HandTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/ZoomTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/LineTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/CurveTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/RectSelectionTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/FillTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/MoveLayerTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/AiSelectTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/GradientTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/FreeTransformTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/VectorEditTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/TextTool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/ai/GenerativeFillEngine.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/ai/OnnxSegEngine.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/ai/UpscaleEngine.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/mesh/MLSSolver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/mesh/TriangulationBuilder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/mesh/GridMeshGenerator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/mesh/EdgeAdaptiveMeshGenerator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/mesh/MeshRenderer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/tools/MeshDeformTool.cpp
)

add_library(paint_core STATIC ${CORE_SOURCES})
set_target_properties(paint_core PROPERTIES
  AUTOMOC ON
)
target_include_directories(paint_core PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(paint_core PUBLIC Qt6::Core)
if(PAINT_USE_ONNX)
  target_compile_definitions(paint_core PUBLIC PAINT_USE_ONNX)
  target_link_libraries(paint_core PUBLIC onnxruntime::onnxruntime)
endif()
if(MSVC)
  target_compile_options(paint_core PUBLIC /utf-8)
endif()

if(PAINT_BUILD_QT_PLATFORM)
  add_library(paint_qt_platform STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/qt/QtImageConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/qt/HighQualityTransform.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/skia/SkiaLayerCache.cpp
  )
  target_compile_definitions(paint_skia_platform PUBLIC PAINT_USE_SKIA)
  target_include_directories(paint_skia_platform PUBLIC ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(paint_skia_platform PUBLIC paint_core unofficial::skia::skia)
  if(MSVC)
    target_compile_options(paint_skia_platform PRIVATE /utf-8)
  endif()
endif()

# ── ComfyUI workflow adapter ─────────────────────────────────────────────────
add_library(paint_comfy_platform STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/comfy/WorkflowDocument.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/comfy/WorkflowBinding.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/comfy/ComfyClient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/comfy/ComfyProcessManager.cpp
)
set_target_properties(paint_comfy_platform PROPERTIES AUTOMOC ON)
target_include_directories(paint_comfy_platform PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(paint_comfy_platform PUBLIC Qt6::Core Qt6::Network)
if(MSVC)
  target_compile_options(paint_comfy_platform PRIVATE /utf-8)
endif()

# ── Dev_Bridge debug server (opt-in) ─────────────────────────────────────────
# Enable with: cmake -DPAINT_DEBUG_SERVER=ON ...
# Adds a local HTTP server (port 9223) for runtime observation by Dev_Bridge.
# Never compiled into release builds unless explicitly set.
option(PAINT_DEBUG_SERVER "Build Dev_Bridge HTTP debug server into the app" OFF)

set(APP_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/commands/CommandRegistry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/AppController.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/LpaExporter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/LpaImporter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/PsdExporter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/PsdImporter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/ui/ToolDescriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/ui/UiState.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/ui/IconLoader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/mainwindow/MainWindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/mainwindow/DocumentWorkspace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/canvasview/CanvasWidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/canvasview/SelectionOverlayRenderer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/AdjustmentPropertyPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/AiPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/LayerPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/ToolPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/SubToolPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/ToolPropertyPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/ColorWheelWidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/GenerativeFillDialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/UpscaleDialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/AiModelFolderDialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/WorkflowBindingDialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/ai/WorkflowPresetManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/AiGenerationController.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/AiService.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/ComfyProvider.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/WorkflowFactory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/ComfyUiClient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/bridge/MinimalWebSocket.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/resources/icons.qrc
  ${CMAKE_CURRENT_SOURCE_DIR}/app/panels/MeshDeformPanel.cpp
)



if(PAINT_BUILD_APP)
  if(WIN32)
    list(APPEND APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/app/resources/app.rc)
  endif()
  add_executable(paint_app WIN32 ${APP_SOURCES})
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
      paint_comfy_platform
      Qt6::Core
      Qt6::CorePrivate
      Qt6::Gui
      Qt6::Widgets
      Qt6::Network
      qt6advanceddocking
  )
  if(PAINT_USE_SKIA)
    target_link_libraries(paint_app PRIVATE paint_skia_platform)
    target_compile_definitions(paint_app PRIVATE PAINT_USE_SKIA)
  endif()

  if(PAINT_DEBUG_SERVER)
    target_sources(paint_app PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/app/debug/DebugServer.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/app/debug/DebugActionRegistry.cpp
    )
    target_compile_definitions(paint_app PRIVATE PAINT_DEBUG_SERVER)
    # paint_comfy_platform も同じ定義が必要。ComfyClient.h の #ifdef でクラスレイアウトが
    # 変わるため、全 TU で一致させないと ABI ミスマッチによるクラッシュが起きる。
    target_compile_definitions(paint_comfy_platform PUBLIC PAINT_DEBUG_SERVER)
    # ビルドアイデンティティ: git commit hash を /debug/health に公開する
    execute_process(
      COMMAND git rev-parse --short HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE PAINT_GIT_COMMIT
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    if(NOT PAINT_GIT_COMMIT)
      set(PAINT_GIT_COMMIT "unknown")
    endif()
    target_compile_definitions(paint_app PRIVATE PAINT_GIT_COMMIT="${PAINT_GIT_COMMIT}")
    message(STATUS "Dev_Bridge debug server enabled (port 9223) git=${PAINT_GIT_COMMIT}")
  endif()

  # ── ランタイムリソース: ワークフロー JSON をビルド出力ディレクトリにコピー ──
  set(WORKFLOW_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/app/resources/workflows)
  set(WORKFLOW_DST_DIR $<TARGET_FILE_DIR:paint_app>/resources/workflows)
  add_custom_command(TARGET paint_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${WORKFLOW_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${WORKFLOW_SRC_DIR}/lpa_inpaint_sdxl.json"
      "${WORKFLOW_DST_DIR}/lpa_inpaint_sdxl.json"
    COMMENT "Copying workflow JSON to output directory"
  )
endif()
