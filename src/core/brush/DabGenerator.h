#pragma once

#include <cstdint>
#include <functional>

#include "core/common/FPoint.h"

namespace core {

// ── DabPlacement ─────────────────────────────────────────────────────────────
// DabGenerator が 1 個のスタンプ位置について生成する配置情報。
// center に対する相対オフセットと追加回転角度を保持する。
struct DabPlacement {
  FPoint offset;   ///< scatter による中心からのずれ（無効時は {0,0}）
  float  angle;    ///< angle jitter による追加回転角度 [degrees]（無効時は 0）
};

// ── DabGenerator ─────────────────────────────────────────────────────────────
// stampDabsAt の scatter / angleJitter / dabCount 処理を分離したクラス。
//
// 担当:
//   - LCG 疑似乱数シーケンス管理（決定論的・ストローク開始時にシードリセット）
//   - scatter: 円内一様分布によるスタンプ位置散布
//   - angleJitter: 角度のランダムジッター
//   - dabCount: 1 スタンプ位置あたりの反復回数
//
// 非担当 (BrushTool が保持):
//   - stampAt / blendPixel（実際のピクセル描画）
//   - BrushSettings の読み取り（呼び出し側が引数で渡す）

class DabGenerator {
public:
  using PlacementCallback = std::function<void(const DabPlacement&)>;

  // ストローク開始時にシードをリセットする。
  // seed は決定論的な再現性のためにストローク座標から生成する。
  void beginStroke(uint32_t seed) noexcept;

  // 1 つのスタンプ位置について count 個の DabPlacement を生成しコールバックに渡す。
  // count   : BrushSettings.dynamics.dabCount
  // radius  : 現在の有効半径（scatter 計算に使用）
  // scatter / scatterAmount  : 散布の有効フラグと量（BrushSettings.dynamics.*）
  // angleJitter / amount     : 角度ジッターの有効フラグと量（degrees）
  void generate(
      int   count,
      float radius,
      bool  scatterEnabled, float scatterAmount,
      bool  angleJitter,    float angleJitterAmount,
      const PlacementCallback& callback);

private:
  uint32_t m_seed {0};

  // LCG（BrushTool.cpp の anonymous namespace と同一パラメーター）
  static uint32_t lcgNext(uint32_t& seed) noexcept;
  static float    lcgFloat(uint32_t& seed) noexcept;
};

} // namespace core
