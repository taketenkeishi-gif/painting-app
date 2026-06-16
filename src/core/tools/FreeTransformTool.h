#pragma once

#include <cmath>
#include <functional>

#include "core/buffer/PixelBuffer.h"
#include "core/common/FPoint.h"
#include "core/common/Rect.h"
#include "core/tools/ITool.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// FreeTransformTool  — 自由変形ツール (PS Ctrl+T 相当)
//
// 内部状態:
//   変形パラメータ = 中心(cx,cy) + スケール(sx,sy) + 回転(rot)
//   コーナー位置はこれらから導出
//
// スケール数学:
//   world = Rotate(rot) * Scale(sx,sy) * local + center
//   アンカー固定でスケール変更 → 中心を連動して更新
//
// Qt 委譲:
//   コミット時は QImage::transformed(QTransform, SmoothTransformation) を使用
//   (commitAndGetBuffer は AppController 側で QImage で処理)
// ─────────────────────────────────────────────────────────────────────────────
class FreeTransformTool : public ITool {
public:
  ToolKind         kind()        const noexcept override { return ToolKind::FreeTransform; }
  std::string_view displayName() const noexcept override { return "Free Transform"; }

  ToolResult onPointerPress  (ToolContext& ctx, const ToolPointerEvent& e) override;
  ToolResult onPointerMove   (ToolContext& ctx, const ToolPointerEvent& e) override;
  ToolResult onPointerRelease(ToolContext& ctx, const ToolPointerEvent& e) override;
  ToolResult onCancel        (ToolContext& ctx) override;
  ToolResult onWheel         (ToolContext& ctx, int delta, const ToolPointerEvent& e) override;
  ToolOverlayState overlay() const override;

  // ── セッション管理 ─────────────────────────────────────────────────────────
  void beginSession(PixelBuffer originalBuf, int origOffX, int origOffY,
                    int canvasW, int canvasH);
  bool isActive() const noexcept { return m_active; }
  void cancelSession();

  // ── ズーム注入（ヒットテスト精度）────────────────────────────────────────
  void setZoom(float zoom) noexcept { m_zoom = std::max(0.01f, zoom); }

  // ── 現在の変形パラメータ（Qt 変換行列構築 / ステータス表示）────────────────
  float centerX()     const noexcept { return m_originX + m_tx; }
  float centerY()     const noexcept { return m_originY + m_ty; }
  float scaleX()      const noexcept { return m_sx; }
  float scaleY()      const noexcept { return m_sy; }
  float rotationDeg() const noexcept { return m_rot * (180.f / 3.14159265f); }
  float halfW()       const noexcept { return m_halfW; }
  float halfH()       const noexcept { return m_halfH; }
  float tx()          const noexcept { return m_tx; }
  float ty()          const noexcept { return m_ty; }
  float rot()         const noexcept { return m_rot; }
  float originX()     const noexcept { return m_originX; }
  float originY()     const noexcept { return m_originY; }

  /// キャンバス座標 p をアフィン変換して返す（ベクター点のコミット用）。
  FPoint transformPoint(FPoint p) const noexcept;

  /// 元バッファ（プレビュー / コミット用）
  const PixelBuffer& originalBuffer() const noexcept { return m_origBuf; }
  int origOffsetX() const noexcept { return m_origOffX; }
  int origOffsetY() const noexcept { return m_origOffY; }

  /// スクリーン座標 (target相対) でのヒットテスト
  int hitTestScreen(float sx, float sy) const noexcept;

  // ── Distort mode (Ctrl + corner drag) ────────────────────────────────────
  /// Ctrl + corner drag で透視変換モードに入っているか。
  bool isDistortMode() const noexcept { return m_distortMode; }
  /// 透視変換モード時の4コーナー [TL, TR, BR, BL] キャンバス座標。
  const FPoint* distortCorners() const noexcept { return m_distortCorners; }

private:
  // ハンドル番号
  //   0=TL 1=TC 2=TR
  //   3=ML       4=MR
  //   5=BL 6=BC 7=BR
  //   8=Rotate

  static constexpr int kOpposite[8] = {7,6,5,4,3,2,1,0};

  // コーナーハンドル → corners[] インデックス (TL TR BR BL)
  // 0=TL→0, 2=TR→1, 7=BR→2, 5=BL→3, その他=-1
  static constexpr int kHandleToCornerIdx[8] = {0,-1,1,-1,-1,3,-1,2};

  // ローカル↔キャンバス変換
  void toCanvas(float lx, float ly, float& cx, float& cy) const noexcept;
  void toLocal (float cx, float cy, float& lx, float& ly) const noexcept;

  void updateHandles() noexcept;

  // 状態
  bool        m_active   {false};
  PixelBuffer m_origBuf;
  int         m_origOffX {0}, m_origOffY {0};
  int         m_canvasW  {0}, m_canvasH  {0};

  // 変形パラメータ（ local = original content space ）
  float m_originX {0}, m_originY {0}; ///< content center (canvas px)
  float m_tx {0}, m_ty {0};           ///< 平行移動
  float m_sx {1}, m_sy {1};           ///< スケール
  float m_rot {0};                    ///< 回転 (rad)
  float m_halfW {0}, m_halfH {0};     ///< content の半幅・半高さ (px, 変更しない)

  // ドラッグ状態
  int   m_activeHandle  {-1};
  float m_dragAnchorX   {0}, m_dragAnchorY   {0}; ///< アンカーのキャンバス座標（固定）
  float m_dragAngle0    {0};  ///< 回転ドラッグ開始時の角度
  float m_dragTx0       {0}, m_dragTy0 {0};
  float m_dragSx0       {1}, m_dragSy0 {1};
  float m_dragRot0      {0};

  // ハンドル位置（キャンバス px）
  FPoint m_corners[4]; // TL TR BR BL
  FPoint m_handles[9]; // 0-7 scale, 8 rotate

  float m_zoom {1.f};

  // 透視変換（Distort）モード
  bool   m_distortMode        {false};
  FPoint m_distortCorners[4];     ///< 独立コーナー [TL, TR, BR, BL] キャンバス座標
  FPoint m_dragDistortCorners[4]; ///< ドラッグ開始時の distortCorners スナップショット
  int    m_distortDragCorner  {-1};  ///< ドラッグ中コーナーインデックス (0..3)
};

} // namespace core
