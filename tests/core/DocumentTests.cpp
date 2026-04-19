#include "core/document/Document.h"

#include "TestHelpers.h"

void runDocumentTests() {
  core::Document document(64, 64);
  expectTrue(document.layerCount() == 1, "Document should create one initial layer.");
  expectTrue(document.activeLayerIndex() == 0, "Initial active layer should be index 0.");

  const std::size_t secondLayerIndex = document.addLayer("Layer 2");
  expectTrue(secondLayerIndex == 1, "Second layer index should be 1.");
  expectTrue(document.layerCount() == 2, "Layer count should be 2 after adding.");
  expectTrue(document.activeLayerIndex() == 1, "Added layer should become active.");

  expectTrue(document.setActiveLayer(0), "setActiveLayer should succeed for valid index.");
  expectTrue(document.activeLayerIndex() == 0, "Active layer should move to index 0.");
  expectTrue(!document.setActiveLayer(999), "setActiveLayer should fail for invalid index.");

  expectTrue(document.renameLayer(0, "Background"), "renameLayer should succeed for valid index.");
  expectTrue(document.layerAt(0).name() == "Background", "Layer name should be updated.");
  expectTrue(!document.renameLayer(999, "Invalid"), "renameLayer should fail for invalid index.");
  expectTrue(!document.renameLayer(0, ""), "renameLayer should fail for empty names.");

  document.addLayer("Layer 3");
  expectTrue(document.layerCount() == 3, "Layer count should be 3 after adding third layer.");
  expectTrue(document.activeLayerIndex() == 2, "Third layer should become active.");

  expectTrue(document.removeLayer(1), "removeLayer should remove middle layer.");
  expectTrue(document.layerCount() == 2, "Layer count should be 2 after removal.");
  expectTrue(document.activeLayerIndex() == 1, "Active index should shift after middle deletion.");
  expectTrue(document.layerAt(1).name() == "Layer 3", "Layer order should remain consistent after deletion.");

  expectTrue(document.removeLayer(1), "removeLayer should remove active layer when more than one exists.");
  expectTrue(document.layerCount() == 1, "Layer count should be 1 after second removal.");
  expectTrue(document.activeLayerIndex() == 0, "Active index should move to last remaining layer.");
  expectTrue(!document.removeLayer(0), "removeLayer should fail when attempting to remove the last layer.");
}
