#include "core/color/Color.h"
#include "core/document/Document.h"
#include "core/tools/BrushTool.h"
#include "core/tools/ToolContext.h"

#include "TestHelpers.h"

// ToolContext + イベント経由で 1 ストロークを実行するヘルパー
static void doStroke(core::BrushTool& tool, core::Document& doc,
                     core::Point from, core::Point to) {
    core::PixelBuffer composited(doc.canvasSize().width, doc.canvasSize().height);
    core::ToolContext ctx{doc, composited, tool.settings().color};

    core::ToolPointerEvent ev;
    ev.pressure = 1.0f;

    ev.point  = from;
    ev.fpoint = {static_cast<float>(from.x), static_cast<float>(from.y)};
    tool.onPointerPress(ctx, ev);

    ev.point  = to;
    ev.fpoint = {static_cast<float>(to.x), static_cast<float>(to.y)};
    tool.onPointerMove(ctx, ev);
    tool.onPointerRelease(ctx, ev);
}

void runBrushToolTests() {
    core::BrushTool tool;
    tool.setColor(core::Color{255, 0, 0, 255});
    tool.setSize(1);

    // 赤ブラシで 1 点打ち
    core::Document doc(16, 16);
    doStroke(tool, doc, {4, 4}, {4, 4});
    const int ai = static_cast<int>(doc.activeLayerIndex());
    const core::Color center = doc.layerAt(ai).buffer().pixel(4, 4);
    expectNear(center.r, 255, 0, "Brush should draw red on the target pixel.");
    expectNear(center.g, 0,   0, "Brush red stroke should keep green channel at 0.");
    expectNear(center.b, 0,   0, "Brush red stroke should keep blue channel at 0.");
    expectNear(center.a, 255, 0, "Brush stroke should produce opaque pixel.");

    // 水平線のストローク補間
    core::Document lineDoc(16, 16);
    doStroke(tool, lineDoc, {2, 8}, {10, 8});
    const int lai = static_cast<int>(lineDoc.activeLayerIndex());
    const core::Color mid = lineDoc.layerAt(lai).buffer().pixel(6, 8);
    expectNear(mid.a, 255, 0, "Stroke interpolation should fill middle pixels.");

    // サイズクランプ
    tool.setSize(0);
    expectTrue(tool.settings().size == 1, "Brush size should clamp to at least 1.");

    // 低 opacity
    core::Document opacityDoc(16, 16);
    tool.setOpacity(0.25F);
    doStroke(tool, opacityDoc, {8, 8}, {8, 8});
    const int oai = static_cast<int>(opacityDoc.activeLayerIndex());
    const core::Color lowOpacity = opacityDoc.layerAt(oai).buffer().pixel(8, 8);
    expectTrue(lowOpacity.a < 255, "Brush opacity setting should reduce resulting alpha.");

    // 高 opacity
    tool.setOpacity(1.0F);
    core::Document fullOpacityDoc(16, 16);
    doStroke(tool, fullOpacityDoc, {8, 8}, {8, 8});
    const int fai = static_cast<int>(fullOpacityDoc.activeLayerIndex());
    const core::Color fullOpacity = fullOpacityDoc.layerAt(fai).buffer().pixel(8, 8);
    expectTrue(fullOpacity.a >= lowOpacity.a,
               "Higher brush opacity should not produce weaker alpha than lower opacity.");
}
