#include "core/color/Color.h"
#include "core/document/Document.h"
#include "core/render/Renderer.h"

#include "TestHelpers.h"

void runRendererTests() {
  core::Document document(1, 1);
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
}
