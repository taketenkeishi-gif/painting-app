#include "core/color/Color.h"
#include "core/layer/Layer.h"

#include "TestHelpers.h"

void runLayerTests() {
  core::Layer layer("Layer Test", 8, 8);
  expectTrue(layer.visible(), "Layer should be visible by default.");
  expectNear(static_cast<int>(layer.opacity() * 100.0F), 100, 0, "Layer opacity should default to 1.0.");

  layer.setVisible(false);
  expectTrue(!layer.visible(), "Layer visibility flag should update.");

  layer.setOpacity(1.5F);
  expectNear(static_cast<int>(layer.opacity() * 100.0F), 100, 0, "Opacity should clamp to 1.0.");
  layer.setOpacity(-0.5F);
  expectNear(static_cast<int>(layer.opacity() * 100.0F), 0, 0, "Opacity should clamp to 0.0.");

  core::PixelBuffer buffer(4, 4, core::Color::Transparent());
  expectTrue(buffer.setPixel(1, 1, core::Color {10, 20, 30, 255}), "setPixel should succeed in bounds.");
  const core::Color inside = buffer.pixel(1, 1);
  expectNear(inside.r, 10, 0, "Pixel red channel should persist.");
  expectNear(inside.g, 20, 0, "Pixel green channel should persist.");
  expectNear(inside.b, 30, 0, "Pixel blue channel should persist.");
  expectNear(inside.a, 255, 0, "Pixel alpha channel should persist.");

  expectTrue(!buffer.setPixel(-1, -1, core::Color {1, 1, 1, 255}), "setPixel should fail out of bounds.");
  const core::Color outside = buffer.pixel(-1, -1);
  expectNear(outside.a, 0, 0, "Out of bounds pixel read should return transparent.");
}
