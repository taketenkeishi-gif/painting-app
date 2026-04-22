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

  core::Document orderDoc(32, 32);
  orderDoc.renameLayer(0, "Base");
  orderDoc.addLayer("Mid");
  orderDoc.addLayer("Top");
  expectTrue(orderDoc.moveLayerDown(2), "moveLayerDown should move top layer toward bottom.");
  expectTrue(orderDoc.layerAt(1).name() == "Top", "Layer order should reflect downward move.");
  expectTrue(orderDoc.moveLayerUp(1), "moveLayerUp should move layer back upward.");
  expectTrue(orderDoc.layerAt(2).name() == "Top", "Layer order should reflect upward move.");
  expectTrue(!orderDoc.moveLayerUp(2), "moveLayerUp should fail for topmost layer.");
  expectTrue(!orderDoc.moveLayerDown(0), "moveLayerDown should fail for bottom-most layer.");

  core::Document vectorDoc(48, 48);
  const std::size_t vectorIndex = vectorDoc.addVectorLayer("Vector 1");
  expectTrue(vectorDoc.layerAt(vectorIndex).kind() == core::LayerKind::Vector, "addVectorLayer should create vector kind.");
  const std::size_t folderIndex = vectorDoc.addFolderLayer("Folder 1");
  expectTrue(vectorDoc.layerAt(folderIndex).kind() == core::LayerKind::Folder, "addFolderLayer should create folder kind.");
  vectorDoc.layerAt(vectorIndex).addVectorPath(core::VectorPath {{core::Point {1, 1}, core::Point {10, 10}}, core::Color {255, 0, 0, 255}, 3, 1.0F});
  expectTrue(!vectorDoc.layerAt(vectorIndex).vectorPaths().empty(), "Vector layer should keep vector paths.");
  const std::size_t duplicateIndex = vectorDoc.duplicateLayer(vectorIndex);
  expectTrue(duplicateIndex == vectorIndex + 1, "duplicateLayer should insert next to source.");
  expectTrue(vectorDoc.layerAt(duplicateIndex).kind() == core::LayerKind::Vector, "Duplicated layer should preserve kind.");
  expectTrue(vectorDoc.layerAt(duplicateIndex).vectorPaths().size() == vectorDoc.layerAt(vectorIndex).vectorPaths().size(), "Duplicated vector paths should be copied.");
}
