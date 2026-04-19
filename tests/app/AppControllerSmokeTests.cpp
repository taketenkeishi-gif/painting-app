#include <exception>
#include <iostream>

#include "app/bridge/AppController.h"
#include "core/color/Color.h"
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

    std::cout << "App smoke tests passed.\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "App smoke tests failed: " << ex.what() << '\n';
    return 1;
  }
}
