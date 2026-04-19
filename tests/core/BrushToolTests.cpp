#include "core/color/Color.h"
#include "core/layer/Layer.h"
#include "core/tools/BrushTool.h"

#include "TestHelpers.h"

void runBrushToolTests() {
  core::BrushTool tool;
  tool.setColor(core::Color {255, 0, 0, 255});
  tool.setSize(1);

  core::Layer layer("Brush Test", 16, 16);
  tool.stroke(layer, core::Point {4, 4}, core::Point {4, 4});
  const core::Color center = layer.buffer().pixel(4, 4);
  expectNear(center.r, 255, 0, "Brush should draw red on the target pixel.");
  expectNear(center.g, 0, 0, "Brush red stroke should keep green channel at 0.");
  expectNear(center.b, 0, 0, "Brush red stroke should keep blue channel at 0.");
  expectNear(center.a, 255, 0, "Brush stroke should produce opaque pixel.");

  core::Layer lineLayer("Brush Line", 16, 16);
  tool.stroke(lineLayer, core::Point {2, 8}, core::Point {10, 8});
  const core::Color mid = lineLayer.buffer().pixel(6, 8);
  expectNear(mid.a, 255, 0, "Stroke interpolation should fill middle pixels.");

  tool.setSize(0);
  expectTrue(tool.settings().size == 1, "Brush size should clamp to at least 1.");
}
