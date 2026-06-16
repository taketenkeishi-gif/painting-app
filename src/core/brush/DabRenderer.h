#pragma once

#include <functional>

#include "core/buffer/PixelBuffer.h"
#include "core/color/Color.h"
#include "core/common/FPoint.h"
#include "core/tools/ToolTypes.h"

namespace core {

// stampAt の内部処理（coverage計算・テクスチャ・wetMix/smear・buildup制御・dab pixel loop）を担当。
// blendPixel は BrushTool 側に残す。
class DabRenderer {
public:
  // BrushTool::blendPixel を抽象化したコールバック。
  // 引数: (x, y, drawColor, pixelStrength, lockAlpha)
  using BlendFn = std::function<void(int x, int y, const Color& color, float strength, bool lockAlpha)>;

  // 1 個の dab を buffer に描画する。
  //   smearColor : ストローク中に更新される（in/out）
  //   strokeAccum: buildup=false のときの最大 coverage バッファ
  void render(
      PixelBuffer&         buffer,
      const PixelBuffer&   composited,
      PixelBuffer&         strokeAccum,
      const BrushSettings& settings,
      bool                 maskEditMode,
      Color&               smearColor,
      const FPoint&        center,
      float                radius,
      float                strength,
      bool                 lockAlpha,
      float                angleDegrees,
      const BlendFn&       blendFn);
};

} // namespace core
