#include <exception>
#include <iostream>

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

    // Drawing + color/size reflection + undo/redo
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

    expectTrue(controller.setCurrentTool(core::ToolKind::Line), "Line tool should be selectable.");
    controller.setBrushColor(core::Color {255, 255, 0, 255});
    controller.beginStroke(30, 30);
    controller.continueStroke(36, 36);
    controller.endStroke();
    const core::Color linePixel = controller.document().layerAt(0).buffer().pixel(33, 33);
    expectTrue(linePixel.r > 0 && linePixel.g > 0, "Line tool should draw between press/release points.");

    std::cout << "App smoke tests passed.\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "App smoke tests failed: " << ex.what() << '\n';
    return 1;
  }
}
