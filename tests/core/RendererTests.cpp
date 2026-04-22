#include "core/color/Color.h"
#include "core/document/Document.h"
#include "core/render/Renderer.h"

#include "TestHelpers.h"

void runRendererTests() {
  core::Document document(1, 1);
  document.setPaperVisible(false);
  document.layerAt(0).buffer().setPixel(0, 0, core::Color {255, 0, 0, 255});
  document.addLayer("Top");
  document.layerAt(1).buffer().setPixel(0, 0, core::Color {0, 0, 255, 128});

  core::Renderer renderer;
  const core::PixelBuffer composited = renderer.composite(document);
  const core::Color blended = composited.pixel(0, 0);

  expectNear(blended.r, 127, 1, "Blended red channel should be around 127.");
  expectNear(blended.g, 0, 1, "Blended green channel should stay 0.");
  expectNear(blended.b, 128, 1, "Blended blue channel should be around 128.");
  expectNear(blended.a, 255, 0, "Final alpha should be 255 over opaque base.");

  document.setLayerVisible(1, false);
  const core::Color hiddenTop = renderer.composite(document).pixel(0, 0);
  expectNear(hiddenTop.r, 255, 0, "Hidden top layer should not affect red.");
  expectNear(hiddenTop.g, 0, 0, "Hidden top layer should not affect green.");
  expectNear(hiddenTop.b, 0, 0, "Hidden top layer should not affect blue.");

  core::Document vectorDoc(16, 16);
  vectorDoc.setPaperVisible(false);
  vectorDoc.addVectorLayer("Vector");
  core::Layer& vectorLayer = vectorDoc.layerAt(1);
  vectorLayer.addVectorPath(core::VectorPath {{core::Point {2, 2}, core::Point {13, 13}}, core::Color {0, 255, 0, 255}, 2, 1.0F});
  const core::Color vectorPixel = renderer.composite(vectorDoc).pixel(8, 8);
  expectTrue(vectorPixel.g > 0, "Vector path should rasterize and appear in composited image.");

  core::Document clipDoc(2, 1);
  clipDoc.setPaperVisible(false);
  clipDoc.layerAt(0).buffer().setPixel(0, 0, core::Color {255, 0, 0, 255});
  clipDoc.layerAt(0).buffer().setPixel(1, 0, core::Color::Transparent());
  clipDoc.addLayer("Top");
  clipDoc.layerAt(1).buffer().setPixel(0, 0, core::Color {0, 255, 0, 255});
  clipDoc.layerAt(1).buffer().setPixel(1, 0, core::Color {0, 255, 0, 255});
  clipDoc.layerAt(1).setClippedToBelow(true);
  const core::PixelBuffer clipped = renderer.composite(clipDoc);
  expectTrue(clipped.pixel(1, 0).a == 0, "Clipped layer should not draw where below-alpha is zero.");

  core::Document maskDoc(1, 1);
  maskDoc.setPaperVisible(false);
  maskDoc.layerAt(0).buffer().setPixel(0, 0, core::Color {255, 0, 0, 255});
  maskDoc.addLayer("MaskTop");
  core::Layer& maskTop = maskDoc.layerAt(1);
  maskTop.buffer().setPixel(0, 0, core::Color {0, 0, 255, 255});
  maskTop.createMask();
  maskTop.maskBuffer().setPixel(0, 0, core::Color::Transparent());
  const core::Color masked = renderer.composite(maskDoc).pixel(0, 0);
  expectNear(masked.r, 255, 0, "Masked top pixel should reveal base red.");
  expectNear(masked.b, 0, 0, "Masked top pixel should hide top blue.");

  core::Document folderDoc(1, 1);
  folderDoc.setPaperVisible(false);
  folderDoc.addFolderLayer("Folder");
  folderDoc.layerAt(1).buffer().setPixel(0, 0, core::Color {0, 255, 0, 255});
  const core::Color folderComposite = renderer.composite(folderDoc).pixel(0, 0);
  expectNear(folderComposite.a, 0, 0, "Folder layers should be non-rendering placeholders.");
}
