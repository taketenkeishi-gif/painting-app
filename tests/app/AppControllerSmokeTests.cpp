#include <exception>
#include <iostream>
#include <string>

#include "app/bridge/AppController.h"
#include "core/color/Color.h"
#include "core/tools/ToolType.h"
#include "TestHelpers.h"

int main() {
  try {
    app::bridge::AppController controller(nullptr);

    // Startup
    expectTrue(controller.document().layerCount() == 1, "AppController should start with one layer.");

    // New canvas
    controller.newDocument(320, 240);
    expectTrue(controller.document().canvasSize().width == 320, "New canvas width should be applied.");
    expectTrue(controller.document().canvasSize().height == 240, "New canvas height should be applied.");
    expectTrue(!controller.canUndo(), "Undo should be unavailable right after creating a document.");

    // Layer add/select/rename/visibility/delete
    controller.addLayer();
    expectTrue(controller.document().layerCount() == 2, "Layer add should increase layer count.");
    controller.setActiveLayer(0);
    expectTrue(controller.document().activeLayerIndex() == 0, "Active layer selection should update.");
    expectTrue(controller.renameLayer(0, "Background"), "Rename should succeed for valid index.");
    expectTrue(controller.document().layerAt(0).name() == "Background", "Renamed layer name should persist.");
    controller.setLayerVisible(1, false);
    expectTrue(!controller.document().layerAt(1).visible(), "Visibility toggle should update document state.");
    expectTrue(controller.canUndo(), "Visibility toggle should create an undo entry.");
    expectTrue(controller.undo(), "Undo should restore previous visibility state.");
    expectTrue(controller.document().layerAt(1).visible(), "Visibility should return to previous state after undo.");
    expectTrue(controller.redo(), "Redo should re-apply visibility toggle.");
    expectTrue(!controller.document().layerAt(1).visible(), "Visibility should be toggled again after redo.");
    expectTrue(controller.removeLayer(1), "Delete should remove selected layer when more than one exists.");
    expectTrue(controller.document().layerCount() == 1, "Layer count should decrease after delete.");
    expectTrue(!controller.removeLayer(0), "Deleting the last layer should be rejected.");
    expectTrue(!controller.canUndo(), "Undo should be unavailable after history reset operations.");

    controller.newDocument(32, 32);
    controller.setActiveLayer(0);
    controller.setCurrentTool(core::ToolKind::Brush);
    controller.setBrushColor(core::Color {255, 0, 0, 255});
    controller.beginStroke(6, 6);
    controller.endStroke();
    expectTrue(controller.toggleActiveLayerLock(), "Full layer lock should toggle on.");
    controller.beginStroke(10, 10);
    controller.endStroke();
    expectTrue(
        controller.document().layerAt(0).buffer().pixel(10, 10).a == 0,
        "Locked layer should reject additional brush stroke.");
    expectTrue(controller.toggleActiveLayerLock(), "Full layer lock should toggle off.");
    controller.beginStroke(10, 10);
    controller.endStroke();
    expectTrue(
        controller.document().layerAt(0).buffer().pixel(10, 10).a > 0,
        "Unlocked layer should accept brush stroke again.");
    expectTrue(controller.toggleActiveLayerAlphaLock(), "Alpha lock should toggle on for raster layer.");
    controller.setCurrentTool(core::ToolKind::Fill);
    controller.setBrushColor(core::Color {0, 255, 0, 255});
    controller.beginStroke(0, 0);
    controller.endStroke();
    expectTrue(
        controller.document().layerAt(0).buffer().pixel(0, 0).a == 0,
        "Alpha locked layer should keep transparent pixels unfilled.");
    expectTrue(controller.toggleActiveLayerAlphaLock(), "Alpha lock should toggle off.");
    controller.newDocument(32, 32);
    controller.setActiveLayer(0);
    controller.setCurrentTool(core::ToolKind::Brush);
    controller.setBrushColor(core::Color {255, 0, 0, 255});
    controller.setBrushSize(1);
    controller.beginStroke(4, 4);
    controller.endStroke();
    controller.setCurrentTool(core::ToolKind::MoveLayer);
    expectTrue(controller.toggleActiveLayerPositionLock(), "Position lock should toggle on.");
    controller.beginStroke(0, 0);
    controller.continueStroke(5, 5);
    controller.endStroke();
    expectTrue(
        controller.document().layerAt(0).buffer().pixel(4, 4).a > 0 &&
            controller.document().layerAt(0).buffer().pixel(9, 9).a == 0,
        "Position locked layer should reject move-layer translation.");
    expectTrue(controller.toggleActiveLayerPositionLock(), "Position lock should toggle off.");

    controller.addLayer();
    controller.addLayer();
    controller.setActiveLayer(0);
    expectTrue(controller.moveLayerUp(0), "moveLayerUp should move the active layer up one slot.");
    expectTrue(controller.document().activeLayerIndex() == 1, "Active layer should follow moved layer.");
    expectTrue(controller.canUndo(), "Layer order change should be undoable.");
    expectTrue(controller.undo(), "Undo should restore layer order.");
    expectTrue(controller.document().activeLayerIndex() == 0, "Undo should restore moved layer position.");
    expectTrue(controller.redo(), "Redo should re-apply layer order move.");
    expectTrue(controller.document().activeLayerIndex() == 1, "Redo should move layer again.");
    controller.newDocument(64, 64);
    controller.addLayer();
    controller.addLayer();
    controller.setActiveLayer(2);
    expectTrue(controller.moveLayer(2, 0), "Direct moveLayer should support drag/drop style reorder.");
    expectTrue(controller.document().activeLayerIndex() == 0, "Direct move should keep moved layer active.");
    expectTrue(controller.undo(), "Direct moveLayer should be undoable.");
    expectTrue(controller.document().activeLayerIndex() == 2, "Undo should restore moved layer index.");
    expectTrue(controller.redo(), "Direct moveLayer should be redoable.");
    expectTrue(controller.document().activeLayerIndex() == 0, "Redo should reapply moved layer index.");

    // Drawing + color/size reflection + undo/redo
    controller.setCurrentTool(core::ToolKind::Brush);
    controller.addLayer();
    controller.setActiveLayer(1);
    controller.beginStroke(-100, -100);
    controller.continueStroke(-120, -120);
    controller.endStroke();
    expectTrue(!controller.canUndo(), "No-op stroke should not create a history entry.");

    controller.setBrushColor(core::Color {0, 255, 0, 255});
    controller.setBrushSize(12);
    controller.beginStroke(20, 20);
    controller.continueStroke(40, 20);
    controller.endStroke();

    const core::Color sampled = controller.document().layerAt(1).buffer().pixel(30, 20);
    expectTrue(sampled.g > 0, "Brush color should be reflected in rendered pixels.");
    expectTrue(sampled.a > 0, "Brush stroke should produce visible pixels.");
    expectTrue(controller.toolState().size == 12, "Brush size UI state should stay synchronized.");
    expectTrue(controller.canUndo(), "Undo should be available after a stroke.");

    expectTrue(controller.undo(), "Undo should restore the previous layer state.");
    const core::Color afterUndo = controller.document().layerAt(1).buffer().pixel(30, 20);
    expectTrue(afterUndo.a == 0, "Undo should clear the drawn pixel for this stroke.");

    expectTrue(controller.redo(), "Redo should re-apply the undone stroke.");
    const core::Color afterRedo = controller.document().layerAt(1).buffer().pixel(30, 20);
    expectTrue(afterRedo.g > 0, "Redo should bring the stroke back.");

    controller.beginStroke(60, 30);
    controller.continueStroke(80, 30);
    controller.endStroke();
    controller.beginStroke(100, 30);
    controller.continueStroke(120, 30);
    controller.endStroke();
    expectTrue(controller.undo(), "Second stroke should be undoable.");
    expectTrue(controller.undo(), "First stroke should be undoable.");
    const core::Color multiUndo = controller.document().layerAt(1).buffer().pixel(110, 30);
    expectTrue(multiUndo.a == 0, "Multiple undo operations should revert multiple strokes.");

    // Sub-tool presets + descriptor-driven defaults
    expectTrue(!controller.subToolViewModels().empty(), "Current tool should expose at least one sub-tool.");
    expectTrue(controller.currentSubToolId() == "brush_normal", "Brush should start with the default sub-tool.");
    expectTrue(controller.setCurrentSubTool("brush_airbrush"), "Airbrush sub-tool should be selectable.");
    expectTrue(controller.toolState().size == 24, "Sub-tool default size should be applied.");
    expectTrue(controller.toolState().opacity == 28, "Sub-tool default opacity should be applied.");
    expectTrue(controller.toolState().hardness == 10, "Sub-tool default hardness should be applied.");
    expectTrue(!controller.setCurrentSubTool("invalid_subtool"), "Unknown sub-tool id should be rejected.");

    controller.newDocument(64, 64);
    controller.setActiveLayer(0);
    controller.setCurrentTool(core::ToolKind::Brush);
    controller.setCurrentSubTool("brush_airbrush");
    controller.setBrushColor(core::Color {255, 0, 0, 255});
    controller.beginStroke(10, 10);
    controller.endStroke();
    const core::Color airbrushPixel = controller.document().layerAt(0).buffer().pixel(10, 10);
    expectTrue(airbrushPixel.a > 0 && airbrushPixel.a < 200, "Airbrush preset should produce lower opacity than hard brush.");

    controller.newDocument(64, 64);
    controller.setActiveLayer(0);
    controller.setCurrentTool(core::ToolKind::Brush);
    controller.setCurrentSubTool("brush_hard");
    controller.setBrushColor(core::Color {255, 0, 0, 255});
    controller.beginStroke(10, 10);
    controller.endStroke();
    const core::Color hardBrushPixel = controller.document().layerAt(0).buffer().pixel(10, 10);
    expectTrue(hardBrushPixel.a > airbrushPixel.a, "Hard brush preset should produce stronger alpha than airbrush.");

    // Property update should reflect immediately in brush behavior.
    controller.newDocument(64, 64);
    controller.setActiveLayer(0);
    controller.setCurrentTool(core::ToolKind::Brush);
    controller.setCurrentSubTool("brush_normal");
    controller.setBrushColor(core::Color {0, 0, 255, 255});
    controller.setBrushOpacity(20);
    controller.beginStroke(14, 14);
    controller.endStroke();
    const core::Color lowOpacityPixel = controller.document().layerAt(0).buffer().pixel(14, 14);
    controller.setBrushOpacity(100);
    controller.beginStroke(18, 14);
    controller.endStroke();
    const core::Color highOpacityPixel = controller.document().layerAt(0).buffer().pixel(18, 14);
    expectTrue(highOpacityPixel.a > lowOpacityPixel.a, "Changing opacity via controller should immediately affect stroke result.");

    // Tool switching + Brush / Eraser / Fill / MoveLayer / RectSelection + Undo/Redo
    controller.newDocument(64, 64);
    controller.setActiveLayer(0);

    expectTrue(controller.setCurrentTool(core::ToolKind::Brush), "Brush tool should be selectable.");
    controller.setBrushColor(core::Color {255, 0, 0, 255});
    controller.setBrushSize(4);
    controller.beginStroke(8, 8);
    controller.continueStroke(10, 8);
    controller.endStroke();
    const core::Color brushPixel = controller.document().layerAt(0).buffer().pixel(9, 8);
    expectTrue(brushPixel.r > 0 && brushPixel.a > 0, "Brush tool should paint pixels.");

    expectTrue(controller.setCurrentTool(core::ToolKind::Eraser), "Eraser tool should be selectable.");
    controller.beginStroke(8, 8);
    controller.continueStroke(10, 8);
    controller.endStroke();
    const core::Color erasedPixel = controller.document().layerAt(0).buffer().pixel(9, 8);
    expectTrue(erasedPixel.a == 0, "Eraser tool should clear alpha on painted pixels.");

    expectTrue(controller.setCurrentTool(core::ToolKind::Fill), "Fill tool should be selectable.");
    controller.setBrushColor(core::Color {0, 0, 255, 255});
    controller.beginStroke(0, 0);
    controller.endStroke();
    const core::Color fillPixel = controller.document().layerAt(0).buffer().pixel(0, 0);
    expectTrue(fillPixel.b > 0 && fillPixel.a > 0, "Fill tool should change pixels.");
    expectTrue(controller.undo(), "Fill should be undoable.");
    expectTrue(controller.document().layerAt(0).buffer().pixel(0, 0).a == 0, "Fill undo should restore previous pixel state.");
    expectTrue(controller.redo(), "Fill should be redoable.");
    expectTrue(controller.document().layerAt(0).buffer().pixel(0, 0).b > 0, "Fill redo should restore filled state.");
    expectTrue(controller.currentToolSupportsFillThreshold(), "Fill tool should expose threshold property.");
    expectTrue(controller.currentToolSupportsFillContiguous(), "Fill tool should expose contiguous property.");
    expectTrue(controller.currentToolSupportsFillGapClose(), "Fill tool should expose gap-close property.");

    controller.newDocument(32, 32);
    controller.setActiveLayer(0);
    controller.setCurrentTool(core::ToolKind::Brush);
    controller.setBrushColor(core::Color {255, 0, 0, 255});
    controller.setBrushSize(1);
    controller.beginStroke(4, 4);
    controller.endStroke();
    controller.beginStroke(22, 20);
    controller.endStroke();
    controller.setCurrentTool(core::ToolKind::Fill);
    controller.setCurrentSubTool("fill_default");
    controller.setFillContiguous(false);
    controller.setFillThreshold(0);
    controller.setBrushColor(core::Color {0, 0, 255, 255});
    controller.beginStroke(4, 4);
    controller.endStroke();
    const core::Color firstFilled = controller.document().layerAt(0).buffer().pixel(4, 4);
    const core::Color secondFilled = controller.document().layerAt(0).buffer().pixel(22, 20);
    expectTrue(firstFilled.b > 0 && secondFilled.b > 0, "Non-contiguous fill should recolor disconnected matching regions.");

    expectTrue(controller.setCurrentTool(core::ToolKind::RectSelection), "RectSelection tool should be selectable.");
    controller.beginStroke(4, 4);
    controller.continueStroke(12, 12);
    controller.endStroke();
    expectTrue(controller.document().selection().hasSelection(), "RectSelection should set an active selection.");
    expectTrue(controller.document().selection().contains(8, 8), "Selection mask should include points inside the dragged rect.");
    expectTrue(controller.undo(), "Selection change should be undoable.");
    expectTrue(!controller.document().selection().hasSelection(), "Undo should clear selection to previous state.");
    expectTrue(controller.redo(), "Selection change should be redoable.");
    expectTrue(controller.document().selection().contains(8, 8), "Redo should restore selection.");
    expectTrue(controller.undo(), "Selection should be clearable again before move-layer test.");

    controller.beginStroke(6, 6);
    controller.continueStroke(10, 10);
    controller.endStroke();
    expectTrue(controller.clearSelection(), "Clear Selection should clear current selection.");
    expectTrue(!controller.document().selection().hasSelection(), "Selection should be empty after clear.");
    expectTrue(controller.undo(), "Clear Selection should be undoable.");
    expectTrue(controller.document().selection().hasSelection(), "Undo after clear should restore selection.");
    expectTrue(controller.redo(), "Clear Selection should be redoable.");
    expectTrue(!controller.document().selection().hasSelection(), "Redo after clear should clear selection again.");

    controller.beginStroke(6, 6);
    controller.continueStroke(10, 10);
    controller.endStroke();
    expectTrue(controller.invertSelection(), "Invert Selection should succeed when mask exists.");
    expectTrue(controller.document().selection().contains(0, 0), "Invert Selection should select outside area.");
    expectTrue(controller.undo(), "Invert Selection should be undoable.");
    expectTrue(!controller.document().selection().contains(0, 0), "Undo should restore original selection mask.");
    expectTrue(controller.redo(), "Invert Selection should be redoable.");
    expectTrue(controller.document().selection().contains(0, 0), "Redo should re-apply inverted selection mask.");
    expectTrue(controller.clearSelection(), "Clear selection should work after invert redo.");
    expectTrue(controller.currentToolSupportsSelectionMode(), "Selection tool should expose mode property.");

    controller.newDocument(32, 32);
    controller.setActiveLayer(0);
    controller.setCurrentTool(core::ToolKind::Brush);
    controller.setBrushColor(core::Color {200, 50, 50, 255});
    controller.setBrushSize(1);
    controller.beginStroke(3, 3);
    controller.endStroke();
    controller.beginStroke(24, 24);
    controller.endStroke();
    controller.setCurrentTool(core::ToolKind::RectSelection);
    controller.setCurrentSubTool("auto_select");
    controller.setSelectionMode(app::ui::SelectionMode::AutoSelect);
    controller.setAutoSelectThreshold(8);
    controller.setAutoSelectContiguous(false);
    controller.setAutoSelectReferAllLayers(true);
    controller.beginStroke(3, 3);
    controller.endStroke();
    expectTrue(
        controller.document().selection().contains(3, 3) && controller.document().selection().contains(24, 24),
        "Auto select with non-contiguous option should include disconnected matching colors.");
    expectTrue(controller.undo(), "Auto select selection should be undoable.");
    expectTrue(!controller.document().selection().hasSelection(), "Undo should restore empty selection after auto select.");

    expectTrue(controller.setCurrentTool(core::ToolKind::Brush), "Switching back to Brush should succeed.");
    controller.setBrushColor(core::Color {0, 255, 0, 255});
    controller.beginStroke(20, 20);
    controller.endStroke();

    expectTrue(controller.setCurrentTool(core::ToolKind::MoveLayer), "MoveLayer tool should be selectable.");
    controller.beginStroke(0, 0);
    controller.continueStroke(3, 2);
    controller.endStroke();
    const core::Color movedPixel = controller.document().layerAt(0).buffer().pixel(23, 22);
    expectTrue(movedPixel.g > 0, "MoveLayer should shift painted pixels by drag delta.");

    expectTrue(controller.setCurrentTool(core::ToolKind::Shape), "Shape tool should be selectable.");
    controller.setBrushColor(core::Color {255, 255, 0, 255});
    controller.beginStroke(30, 30);
    controller.continueStroke(36, 36);
    controller.endStroke();
    bool lineFound = false;
    for (int y = 30; y <= 36 && !lineFound; ++y) {
      for (int x = 30; x <= 36; ++x) {
        const core::Color linePixel = controller.document().layerAt(0).buffer().pixel(x, y);
        if (linePixel.r > 0 && linePixel.g > 0 && linePixel.a > 0) {
          lineFound = true;
          break;
        }
      }
    }
    expectTrue(lineFound, "Line tool should draw between press/release points.");

    // Vector layer workflow + layer-kind gated tools
    controller.newDocument(64, 64);
    controller.addVectorLayer();
    controller.setActiveLayer(1);
    expectTrue(controller.document().layerAt(1).kind() == core::LayerKind::Vector, "Vector layer should be creatable from controller.");
    expectTrue(
        controller.activeLayerKindDisplayName() == u8"\u30D9\u30AF\u30BF\u30FC",
        "Active layer kind label should reflect vector layer.");

    controller.setCurrentTool(core::ToolKind::Brush);
    expectTrue(!controller.canUseCurrentToolOnActiveLayer(), "Brush should be restricted on vector layer.");
    controller.setBrushColor(core::Color {255, 0, 0, 255});
    const core::Color vectorBrushBefore = controller.compositedBuffer().pixel(10, 8);
    controller.beginStroke(8, 8);
    controller.continueStroke(14, 8);
    controller.endStroke();
    const core::Color vectorBrushAttempt = controller.compositedBuffer().pixel(10, 8);
    expectTrue(
        vectorBrushAttempt.r == vectorBrushBefore.r &&
            vectorBrushAttempt.g == vectorBrushBefore.g &&
            vectorBrushAttempt.b == vectorBrushBefore.b &&
            vectorBrushAttempt.a == vectorBrushBefore.a,
        "Raster-only brush should not draw on vector layer.");

    controller.setCurrentTool(core::ToolKind::Shape);
    expectTrue(controller.setCurrentSubTool("line_vector"), "Vector line sub-tool should be selectable.");
    expectTrue(controller.canUseCurrentToolOnActiveLayer(), "Vector line should be available on vector layer.");
    controller.setBrushColor(core::Color {0, 255, 255, 255});
    controller.beginStroke(6, 6);
    controller.continueStroke(20, 20);
    controller.endStroke();
    expectTrue(!controller.document().layerAt(1).vectorPaths().empty(), "Line on vector layer should store vector paths.");
    const core::Color vectorLinePixel = controller.compositedBuffer().pixel(12, 12);
    expectTrue(vectorLinePixel.g > 0 && vectorLinePixel.b > 0, "Vector line should appear in composited output.");
    expectTrue(controller.undo(), "Vector line stroke should be undoable.");
    expectTrue(controller.document().layerAt(1).vectorPaths().empty(), "Undo should remove vector path.");
    expectTrue(controller.redo(), "Vector line stroke should be redoable.");
    expectTrue(!controller.document().layerAt(1).vectorPaths().empty(), "Redo should restore vector path.");
    expectTrue(controller.setCurrentSubTool("line_vector_snap"), "Vector snap preset should be selectable.");
    controller.setLineSnapAngle(45);
    controller.beginStroke(10, 10);
    controller.continueStroke(20, 12);
    controller.endStroke();
    const auto& snappedPath = controller.document().layerAt(1).vectorPaths().back();
    expectTrue(snappedPath.points.size() >= 2, "Vector snap stroke should produce at least two points.");
    expectTrue(snappedPath.points[0].y == snappedPath.points[1].y || snappedPath.points[0].x == snappedPath.points[1].x ||
                   std::abs(snappedPath.points[1].x - snappedPath.points[0].x) ==
                       std::abs(snappedPath.points[1].y - snappedPath.points[0].y),
               "Snap angle should quantize vector line direction.");
    controller.setCurrentTool(core::ToolKind::Eraser);
    expectTrue(controller.setCurrentSubTool("eraser_vector_touch"), "Vector eraser touched preset should be selectable.");
    expectTrue(controller.canUseCurrentToolOnActiveLayer(), "Vector eraser should be available on vector layer.");
    const std::size_t pathCountBeforeErase = controller.document().layerAt(1).vectorPaths().size();
    controller.beginStroke(6, 6);
    controller.continueStroke(24, 24);
    controller.endStroke();
    const std::size_t pathCountAfterErase = controller.document().layerAt(1).vectorPaths().size();
    expectTrue(pathCountAfterErase < pathCountBeforeErase, "Vector eraser should remove touched vector paths.");
    expectTrue(controller.undo(), "Vector erase should be undoable.");
    expectTrue(
        controller.document().layerAt(1).vectorPaths().size() == pathCountBeforeErase,
        "Undo should restore erased vector paths.");
    expectTrue(controller.redo(), "Vector erase should be redoable.");
    expectTrue(
        controller.document().layerAt(1).vectorPaths().size() == pathCountAfterErase,
        "Redo should remove vector paths again.");
    expectTrue(controller.setCurrentSubTool("eraser_vector_intersection"), "Vector eraser intersection preset should be selectable.");
    expectTrue(controller.currentToolSupportsVectorEraseMode(), "Vector eraser mode property should be exposed.");
    controller.setVectorEraseMode(app::ui::VectorEraserMode::ToIntersection);
    controller.beginStroke(10, 10);
    controller.continueStroke(22, 22);
    controller.endStroke();
    expectTrue(controller.canUndo(), "Intersection mode erase should create undo history.");
    expectTrue(controller.setCurrentSubTool("eraser_vector_trim"), "Vector eraser trim preset should be selectable.");
    expectTrue(controller.currentToolSupportsVectorTrimOutside(), "Vector trim property should be exposed.");
    controller.setVectorTrimOutside(true);
    expectTrue(controller.toolState().vectorTrimOutside, "Vector trim setting should be reflected in tool state.");

    const auto brushSubToolsOnVector = controller.subToolViewModels();
    expectTrue(!brushSubToolsOnVector.empty(), "Sub tool list should remain available on vector layer.");

    // Sub-tool management operations
    controller.setCurrentTool(core::ToolKind::Brush);
    const std::string originalSubTool = controller.currentSubToolId();
    expectTrue(controller.createCurrentSubTool(), "Create sub-tool should create a new editable preset.");
    const std::string createdSubTool = controller.currentSubToolId();
    expectTrue(createdSubTool != originalSubTool, "Created preset should become active.");
    expectTrue(controller.duplicateCurrentSubTool(), "Duplicate sub-tool should create a new editable preset.");
    const std::string duplicatedSubTool = controller.currentSubToolId();
    expectTrue(duplicatedSubTool != originalSubTool, "Duplicated preset should become active with new id.");
    expectTrue(controller.renameCurrentSubTool("My Brush"), "Rename sub-tool should update display name.");
    expectTrue(controller.currentSubToolDisplayName() == "My Brush", "Renamed sub-tool name should be reflected.");
    expectTrue(controller.deleteCurrentSubTool(), "Delete sub-tool should remove duplicated preset.");
    expectTrue(controller.currentSubToolId() != duplicatedSubTool, "After delete, active sub-tool should switch to an existing one.");
    expectTrue(controller.setCurrentSubTool("brush_hard"), "Built-in hard brush sub-tool should be selectable.");
    controller.setBrushSize(77);
    expectTrue(controller.saveSubToolSettings(), "Sub-tool settings save should succeed.");
    expectTrue(controller.resetCurrentSubTool(), "Reset sub-tool should restore default descriptor values.");
    expectTrue(controller.toolState().size == 6, "Reset hard brush should restore default size.");

    controller.newDocument(40, 30);
    expectTrue(controller.selectAll(), "Select All should create a full-canvas selection.");
    expectTrue(controller.document().selection().contains(0, 0), "Select All should include origin pixel.");
    expectTrue(controller.deselect(), "Deselect should clear active selection.");
    expectTrue(!controller.document().selection().hasSelection(), "Selection should be cleared after deselect.");

    controller.addVectorLayer();
    controller.setActiveLayer(1);
    controller.setCurrentTool(core::ToolKind::Shape);
    controller.setCurrentSubTool("line_vector");
    controller.beginStroke(2, 2);
    controller.continueStroke(20, 2);
    controller.endStroke();
    expectTrue(controller.document().layerAt(1).kind() == core::LayerKind::Vector, "Vector layer should keep vector kind before rasterize.");
    expectTrue(controller.rasterizeActiveLayer(), "Rasterize active layer should convert vector to raster.");
    expectTrue(controller.document().layerAt(1).kind() == core::LayerKind::Raster, "Rasterize should switch layer kind to raster.");
    expectTrue(controller.mergeActiveLayerDown(), "Merge down should succeed for non-bottom layer.");
    expectTrue(controller.document().layerCount() == 1, "Merge down should reduce layer count by one.");

    std::cout << "App smoke tests passed.\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "App smoke tests failed: " << ex.what() << '\n';
    return 1;
  }
}
