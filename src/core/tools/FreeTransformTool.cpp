#include "core/tools/FreeTransformTool.h"

#include <algorithm>
#include <cmath>

#include "core/tools/ToolContext.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// 座標変換
// ─────────────────────────────────────────────────────────────────────────────
void FreeTransformTool::toCanvas(float lx, float ly, float& cx, float& cy) const noexcept {
  const float sx  = lx * m_sx;
  const float sy  = ly * m_sy;
  const float cosR = std::cos(m_rot), sinR = std::sin(m_rot);
  cx = cosR * sx - sinR * sy + m_originX + m_tx;
  cy = sinR * sx + cosR * sy + m_originY + m_ty;
}

void FreeTransformTool::toLocal(float cx, float cy, float& lx, float& ly) const noexcept {
  const float dx = cx - (m_originX + m_tx);
  const float dy = cy - (m_originY + m_ty);
  const float cosR = std::cos(-m_rot), sinR = std::sin(-m_rot);
  const float rx = cosR * dx - sinR * dy;
  const float ry = sinR * dx + cosR * dy;
  lx = (m_sx == 0.f ? 0.f : rx / m_sx);
  ly = (m_sy == 0.f ? 0.f : ry / m_sy);
}

// ─────────────────────────────────────────────────────────────────────────────
// ハンドル位置の更新
// ─────────────────────────────────────────────────────────────────────────────
void FreeTransformTool::updateHandles() noexcept {
  auto C = [&](float lx, float ly) -> FPoint {
    float cx, cy; toCanvas(lx, ly, cx, cy); return {cx, cy};
  };
  m_corners[0] = C(-m_halfW, -m_halfH); // TL
  m_corners[1] = C(+m_halfW, -m_halfH); // TR
  m_corners[2] = C(+m_halfW, +m_halfH); // BR
  m_corners[3] = C(-m_halfW, +m_halfH); // BL

  auto mid = [](FPoint a, FPoint b) -> FPoint { return {(a.x+b.x)*.5f,(a.y+b.y)*.5f}; };
  m_handles[0] = m_corners[0];
  m_handles[1] = mid(m_corners[0], m_corners[1]); // TC
  m_handles[2] = m_corners[1];
  m_handles[3] = mid(m_corners[0], m_corners[3]); // ML
  m_handles[4] = mid(m_corners[1], m_corners[2]); // MR
  m_handles[5] = m_corners[3];
  m_handles[6] = mid(m_corners[2], m_corners[3]); // BC
  m_handles[7] = m_corners[2];

  // 回転ハンドル: TC の上方向 (回転後の法線)
  const FPoint& tc = m_handles[1];
  const float cosR = std::cos(m_rot), sinR = std::sin(m_rot);
  constexpr float kRotDist = 24.f / 1.f; // canvas px (zoom 補正は CanvasWidget 側)
  m_handles[8] = { tc.x - sinR * kRotDist / m_zoom,
                   tc.y - cosR * kRotDist / m_zoom };
}

// ─────────────────────────────────────────────────────────────────────────────
// ヒットテスト（スクリーン座標、target 相対）
// ─────────────────────────────────────────────────────────────────────────────
int FreeTransformTool::hitTestScreen(float sx, float sy) const noexcept {
  if (!m_active) return -1;
  constexpr float kHitR = 10.f; // スクリーン px
  for (int i = 8; i >= 0; --i) {
    const float hx = m_handles[i].x * m_zoom;
    const float hy = m_handles[i].y * m_zoom;
    const float dx = sx - hx, dy = sy - hy;
    if (dx*dx + dy*dy <= kHitR*kHitR) return i;
  }
  // 内部チェック（キャンバス座標に変換）
  float lx, ly; toLocal(sx / m_zoom, sy / m_zoom, lx, ly);
  if (lx >= -m_halfW && lx <= m_halfW && ly >= -m_halfH && ly <= m_halfH)
    return -2;
  return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// セッション管理
// ─────────────────────────────────────────────────────────────────────────────
void FreeTransformTool::beginSession(PixelBuffer originalBuf,
                                      int origOffX, int origOffY,
                                      int canvasW, int canvasH) {
  m_origBuf   = std::move(originalBuf);
  m_origOffX  = origOffX;
  m_origOffY  = origOffY;
  m_canvasW   = canvasW;
  m_canvasH   = canvasH;

  m_tx = m_ty = 0.f;
  m_sx = m_sy = 1.f;
  m_rot = 0.f;

  m_halfW   = static_cast<float>(m_origBuf.width())  * 0.5f;
  m_halfH   = static_cast<float>(m_origBuf.height()) * 0.5f;
  m_originX = static_cast<float>(m_origOffX) + m_halfW;
  m_originY = static_cast<float>(m_origOffY) + m_halfH;

  m_active = true;
  m_activeHandle = -1;
  updateHandles();
}

void FreeTransformTool::cancelSession() {
  m_active = false;
  m_activeHandle = -1;
}

FPoint FreeTransformTool::transformPoint(FPoint p) const noexcept {
  const float lx = p.x - m_originX;
  const float ly = p.y - m_originY;
  float cx, cy;
  toCanvas(lx, ly, cx, cy);
  return {cx, cy};
}

// ─────────────────────────────────────────────────────────────────────────────
// ITool — ポインターイベント
// ─────────────────────────────────────────────────────────────────────────────
ToolResult FreeTransformTool::onPointerPress(ToolContext& ctx, const ToolPointerEvent& e) {
  static_cast<void>(ctx);
  if (!m_active) return {};

  const float cx = e.fpoint.x, cy = e.fpoint.y;

  // スクリーン座標 (target相対) に変換してヒットテスト
  // paintEvent 側でも同じ変換を使う → canvas px * zoom が screen px (target 相対)
  const float sx = cx * m_zoom, sy = cy * m_zoom;
  m_activeHandle = hitTestScreen(sx, sy);

  m_dragTx0   = m_tx;  m_dragTy0  = m_ty;
  m_dragSx0   = m_sx;  m_dragSy0  = m_sy;
  m_dragRot0  = m_rot;

  if (m_activeHandle == -2) {
    // 移動: ドラッグ開始時の canvas 座標を記録
    m_dragAnchorX = cx;
    m_dragAnchorY = cy;
  } else if (m_activeHandle >= 0 && m_activeHandle <= 7) {
    // スケールドラッグ: アンカーのキャンバス座標を記録
    const int opp = kOpposite[m_activeHandle];
    m_dragAnchorX = m_handles[opp].x;
    m_dragAnchorY = m_handles[opp].y;
  } else if (m_activeHandle == 8) {
    // 回転: 開始角度を記録
    m_dragAngle0 = std::atan2(cy - (m_originY + m_ty),
                               cx - (m_originX + m_tx));
  }

  ToolResult r; r.viewportChanged = true; return r;
}

ToolResult FreeTransformTool::onPointerMove(ToolContext& ctx, const ToolPointerEvent& e) {
  static_cast<void>(ctx);
  if (!m_active || m_activeHandle == -1) return {};

  const float cx = e.fpoint.x, cy = e.fpoint.y;

  if (m_activeHandle == -2) {
    // ── 移動 ──────────────────────────────────────────────────────────────
    // press 時の tx/ty からの差分を加算
    // dragTx0/dragTy0 = press 時の tx/ty
    // press 時の canvas 座標 = m_origOffX + m_halfW + m_dragTx0 (= originX + tx0 ≈ center)
    // 新しい center = cx,cy ではなく、press 時からのデルタで動かす
    // press 時の canvas 座標をドラッグ開始時の center として記録し直す
    // ここでは onPointerPress で m_dragAnchorX/Y にドラッグ開始座標を記録する
    m_tx = m_dragTx0 + (cx - m_dragAnchorX);
    m_ty = m_dragTy0 + (cy - m_dragAnchorY);

  } else if (m_activeHandle == 8) {
    // ── 回転 ──────────────────────────────────────────────────────────────
    const float angle = std::atan2(cy - (m_originY + m_ty),
                                    cx - (m_originX + m_tx));
    m_rot = m_dragRot0 + (angle - m_dragAngle0);

  } else if (m_activeHandle >= 0 && m_activeHandle <= 7) {
    // ── スケール（正しいアンカー固定数学）────────────────────────────────
    //
    // 変換: world = R * diag(sx,sy) * local + center
    // アンカー(opposite handle) は固定
    // mouse は新しい handle 位置
    //
    // mouse - anchor = R * diag(newSx-opp, newSy-opp) - R * diag(sx0-opp, sy0-opp)
    // ローカル軸への投影でスケールを求める

    const float dx = cx - m_dragAnchorX;
    const float dy = cy - m_dragAnchorY;
    const float cosR = std::cos(m_rot), sinR = std::sin(m_rot);
    // キャンバス差分をローカル軸に投影
    const float dLocalX = cosR * dx + sinR * dy;
    const float dLocalY = -sinR * dx + cosR * dy;

    // ハンドルに対応する「ローカル符号」テーブル
    // 各ハンドルの local 座標は: (sxSign * halfW, sySign * halfH)
    // TL=(-1,-1), TC=(0,-1), TR=(+1,-1), ML=(-1,0), MR=(+1,0),
    // BL=(-1,+1), BC=(0,+1), BR=(+1,+1)
    static constexpr int kSxSign[8] = {-1, 0, +1, -1, +1, -1,  0, +1};
    static constexpr int kSySign[8] = {-1,-1,  -1, 0,   0, +1, +1, +1};

    const int sxS = kSxSign[m_activeHandle];
    const int syS = kSySign[m_activeHandle];

    // アンカーの opposite handle の local 座標 = -handle の local 座標
    // anchorLocalX = -sxS * halfW
    // handle が動いた後の new local 距離 = (dLocalX から anchor ローカル距離を引いた分)
    // handle local X = sxS * newHalfW * newSx...
    //
    // 実際には:
    //   dLocalX = 2 * sxS * m_halfW * newSx  (handle - anchor のローカル X)
    //   (anchor は -sxS*halfW*sx0, handle は +sxS*halfW*newSx)
    if (sxS != 0 && m_halfW > 0.5f) {
      const float newSx = dLocalX / (2.f * static_cast<float>(sxS) * m_halfW);
      if (std::abs(newSx) > 0.01f) m_sx = newSx;
    }
    if (syS != 0 && m_halfH > 0.5f) {
      const float newSy = dLocalY / (2.f * static_cast<float>(syS) * m_halfH);
      if (std::abs(newSy) > 0.01f) m_sy = newSy;
    }

    // コーナーハンドルはデフォルトで縦横比固定（Shift で自由スケール解除）
    // dragAspect: ドラッグ開始時の sx/sy 比を使う（初期は 1/1 = 1 → ジャンプなし）
    if (sxS != 0 && syS != 0 && !e.shift) {
      const float dragAspect = (m_dragSy0 == 0.f) ? 1.f : (m_dragSx0 / m_dragSy0);
      if (std::abs(m_sx) > std::abs(m_sy) * dragAspect)
        m_sy = std::copysign(m_sx / dragAspect, m_sy);
      else
        m_sx = std::copysign(m_sy * dragAspect, m_sx);
    }

    // アンカーが canvas 座標で固定されるよう中心を補正。
    if (sxS != 0 && syS != 0) {
      // コーナー: 縦横比固定後の sx/sy で対角アンカーを厳密に固定。
      // anchor = center + R * (-sxS*sx*halfW, -syS*sy*halfH)
      // => center = anchor - R * (-sxS*sx*halfW, -syS*sy*halfH)
      //           = anchor + R * ( sxS*sx*halfW,  syS*sy*halfH)
      const float fsxS = static_cast<float>(sxS);
      const float fsyS = static_cast<float>(syS);
      m_tx = m_dragAnchorX - m_originX
           + cosR * fsxS * m_sx * m_halfW
           - sinR * fsyS * m_sy * m_halfH;
      m_ty = m_dragAnchorY - m_originY
           + sinR * fsxS * m_sx * m_halfW
           + cosR * fsyS * m_sy * m_halfH;
    } else {
      // エッジハンドル: 対応しない軸を固定。
      if (sxS != 0) {
        m_tx = (cx + m_dragAnchorX) * 0.5f - m_originX;
      } else {
        m_tx = m_dragTx0;  // TC / BC: x 方向は固定
      }
      if (syS != 0) {
        m_ty = (cy + m_dragAnchorY) * 0.5f - m_originY;
      } else {
        m_ty = m_dragTy0;  // ML / MR: y 方向は固定
      }
    }
  }

  updateHandles();
  ToolResult r; r.viewportChanged = true; return r;
}

ToolResult FreeTransformTool::onPointerRelease(ToolContext& ctx, const ToolPointerEvent& e) {
  static_cast<void>(ctx); static_cast<void>(e);
  m_activeHandle = -1;
  ToolResult r; r.viewportChanged = true; return r;
}

ToolResult FreeTransformTool::onCancel(ToolContext& ctx) {
  static_cast<void>(ctx);
  cancelSession();
  ToolResult r; r.viewportChanged = true; return r;
}

ToolResult FreeTransformTool::onWheel(ToolContext& ctx, int delta, const ToolPointerEvent& e) {
  static_cast<void>(ctx); static_cast<void>(delta); static_cast<void>(e);
  return {};
}

ToolOverlayState FreeTransformTool::overlay() const {
  ToolOverlayState state;
  if (!m_active) return state;
  state.hasTransformBox = true;
  for (int i = 0; i < 4; ++i) state.transformCorners[i] = m_corners[i];
  for (int i = 0; i < 9; ++i) state.transformHandles[i] = m_handles[i];
  state.transformActiveHandle = m_activeHandle;
  return state;
}

} // namespace core
