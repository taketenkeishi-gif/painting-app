#pragma once

#include <algorithm>
#include <functional>
#include <vector>

#include "core/selection/SelectionMask.h"
#include "core/tools/ITool.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// AiSelectTool  — AI オブジェクト選択ツール
//
// クリックポイントから画像解析で対象オブジェクトを自動選択する。
// ・Stub モード: エッジ検出 + 分散適応フラッドフィルで即時選択
// ・ComfyUI モード: SAM2 推論結果で精密マスク（非同期、AppController 経由）
//
// 使い方:
//   ① 左クリック  → ポジティブポイント追加（対象に含める）
//   ② Shift+左クリック → ネガティブポイント追加（対象から除外）
//   ③ Ctrl+左クリック → 選択解除
// ─────────────────────────────────────────────────────────────────────────────
class AiSelectTool : public ITool {
public:
  struct Settings {
    int  threshold       {24};   ///< 色許容範囲 0–255（スタブ使用時）
    bool referAllLayers  {true}; ///< 合成レイヤーを参照
    bool antiAlias       {true}; ///< エッジをぼかして滑らかに
    bool addMode         {false};///< 既存選択に追加（OR）
    bool subtractMode    {false};///< 既存選択から削除（AND NOT）
    // SAM2 granularity: 0=最小領域(髪など) 1=中 2=オブジェクト 3=被写体全体
    int  granularity     {1};
  };

  /// AppController から ComfyUI 推論コールバックを注入するための型
  using InferenceCallback =
      std::function<void(const PixelBuffer& composited,
                         const std::vector<Point>& positivePoints,
                         const std::vector<Point>& negativePoints)>;

  ToolKind        kind()        const noexcept override { return ToolKind::AiSelect; }
  std::string_view displayName() const noexcept override { return "AI Select"; }

  ToolResult onPointerPress  (ToolContext& ctx, const ToolPointerEvent& e) override;
  ToolResult onPointerMove   (ToolContext& ctx, const ToolPointerEvent& e) override;
  ToolResult onPointerRelease(ToolContext& ctx, const ToolPointerEvent& e) override;
  ToolResult onCancel        (ToolContext& ctx) override;
  ToolResult onWheel         (ToolContext& ctx, int deltaSteps, const ToolPointerEvent& e) override;

  ToolOverlayState overlay() const override;

  // ── 設定 ──────────────────────────────────────────────────────────────────
  void setThreshold      (int v)  noexcept { m_settings.threshold      = std::clamp(v, 0, 255); }
  void setReferAllLayers (bool v) noexcept { m_settings.referAllLayers = v; }
  void setAntiAlias      (bool v) noexcept { m_settings.antiAlias      = v; }
  void setAddMode        (bool v) noexcept { m_settings.addMode        = v; }
  void setSubtractMode   (bool v) noexcept { m_settings.subtractMode   = v; }
  void setGranularity    (int v)  noexcept { m_settings.granularity    = std::clamp(v, 0, 3); }
  const Settings& settings() const noexcept { return m_settings; }

  /// ComfyUI 推論コールバックを注入（nullptr = stub only）
  void setInferenceCallback(InferenceCallback cb) { m_inferenceCallback = std::move(cb); }

  /// AppController からの非同期推論結果を受け取り選択マスクに適用
  void applyInferenceResult(SelectionMask result) { m_pendingResult = std::move(result); }
  bool hasPendingResult() const noexcept { return m_pendingResult.hasSelection() || m_resultReady; }
  SelectionMask takePendingResult() {
    m_resultReady = false;
    return std::move(m_pendingResult);
  }

  const std::vector<Point>& positivePoints() const noexcept { return m_positivePoints; }
  const std::vector<Point>& negativePoints() const noexcept { return m_negativePoints; }

private:
  /// スタブ推論: エッジ検出 + 分散適応フラッドフィル
  SelectionMask runStubSegmentation(const PixelBuffer& source,
                                    const SelectionMask& currentSelection,
                                    const std::vector<Point>& positivePoints,
                                    const std::vector<Point>& negativePoints) const;

  /// ピクセルのソーベル勾配大きさを計算
  static float sobelMagnitude(const PixelBuffer& buf, int x, int y) noexcept;

  /// 色距離（Luma 重みつき）
  static float colorDist(const Color& a, const Color& b) noexcept;

  Settings          m_settings;
  InferenceCallback m_inferenceCallback;
  SelectionMask     m_pendingResult;
  bool              m_resultReady  {false};
  bool              m_selecting    {false};

  std::vector<Point> m_positivePoints;
  std::vector<Point> m_negativePoints;
};

} // namespace core
