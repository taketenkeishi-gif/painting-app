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
  if (m_distortMode) {
    // distort モード: コーナーは m_distortCorners から直接読む
    m_corners[0] = m_distortCorners[0]; // TL
    m_corners[1] = m_distortCorners[1]; // TR
    m_corners[2] = m_distortCorners[2]; // BR
    m_corners[3] = m_distortCorners[3]; // BL
  } else {
    auto C = [&](float lx, float ly) -> FPoint {
      float cx, cy; toCanvas(lx, ly, cx, cy); return {cx, cy};
    };
    m_corners[0] = C(-m_halfW, -m_halfH); // TL
    m_corners[1] = C(+m_halfW, -m_halfH); // TR
    m_corners[2] = C(+m_halfW, +m_halfH); // BR
    m_corners[3] = C(-m_halfW, +m_halfH); // BL
  }

  auto mid = [](FPoint a, FPoint b) -> FPoint { return {(a.x+b.x)*.5f,(a.y+b.y)*.5f}; };
  m_handles[0] = m_corners[0];
  m_handles[1] = mid(m_corners[0], m_corners[1]); // TC
  m_handles[2] = m_corners[1];
  m_handles[3] = mid(m_corners[0], m_corners[3]); // ML
  m_handles[4] = mid(m_corners[1], m_corners[2]); // MR
  m_handles[5] = m_corners[3];
  m_handles[6] = mid(m_corners[2], m_corners[3]); // BC
  m_handles[7] = m_corners[2];

  // 回転ハンドル: TC の上方向
  const FPoint& tc = m_handles[1];
  constexpr float kRotDist = 24.f;
  if (m_distortMode) {
    // 上辺法線から計算
    const float ex = m_corners[1].x - m_corners[0].x;
    const float ey = m_corners[1].y - m_corners[0].y;
    const float elen = std::sqrt(ex*ex + ey*ey);
    if (elen > 0.01f) {
      // 上辺の外側法線 (上向き)
      const float nx =  ey / elen;
      const float ny = -ex / elen;
      m_handles[8] = { tc.x + nx * kRotDist / m_zoom,
                       tc.y + ny * kRotDist / m_zoom };
    } else {
      m_handles[8] = { tc.x, tc.y - kRotDist / m_zoom };
    }
  } else {
    const float cosR = std::cos(m_rot), sinR = std::sin(m_rot);
    m_handles[8] = { tc.x - sinR * kRotDist / m_zoom,
                     tc.y - cosR * kRotDist / m_zoom };
  }
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

  // distort モードリセット
  m_distortMode       = false;
  m_distortDragCorner = -1;

  updateHandles();

  // distort コーナーをアフィン計算後のコーナーで初期化
  for (int i = 0; i < 4; ++i)
    m_distortCorners[i] = m_corners[i];
}

void FreeTransformTool::cancelSession() {
  m_active        = false;
  m_activeHandle  = -1;
  m_distortMode   = false;
  m_distortDragCorner = -1;
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
  const float sx = cx * m_zoom, sy = cy * m_zoom;
  m_activeHandle = hitTestScreen(sx, sy);

  m_dragTx0   = m_tx;  m_dragTy0  = m_ty;
  m_dragSx0   = m_sx;  m_dragSy0  = m_sy;
  m_dragRot0  = m_rot;
  m_distortDragCorner = -1;
  // ドラッグ開始時の distort コーナーを保存
  for (int i = 0; i < 4; ++i) m_dragDistortCorners[i] = m_distortCorners[i];

  // コーナーハンドル (0, 2, 5, 7) で Ctrl または既に distort モード
  const int ci = (m_activeHandle >= 0 && m_activeHandle <= 7)
                   ? kHandleToCornerIdx[m_activeHandle]
                   : -1;
  if (ci >= 0 && (e.ctrl || m_distortMode)) {
    if (!m_distortMode) {
      // アフィンコーナーから distort コーナーを初期化してモードに入る
      m_distortMode = true;
      for (int i = 0; i < 4; ++i) {
        m_distortCorners[i]     = m_corners[i];
        m_dragDistortCorners[i] = m_corners[i];
      }
      updateHandles();
    }
    m_distortDragCorner = ci;
    ToolResult r; r.viewportChanged = true; return r;
  }

  if (m_activeHandle == -2) {
    m_dragAnchorX = cx;
    m_dragAnchorY = cy;
  } else if (m_activeHandle >= 0 && m_activeHandle <= 7) {
    const int opp = kOpposite[m_activeHandle];
    m_dragAnchorX = m_handles[opp].x;
    m_dragAnchorY = m_handles[opp].y;
  } else if (m_activeHandle == 8) {
    m_dragAngle0 = std::atan2(cy - (m_originY + m_ty),
                               cx - (m_originX + m_tx));
  }

  ToolResult r; r.viewportChanged = true; return r;
}

ToolResult FreeTransformTool::onPointerMove(ToolContext& ctx, const ToolPointerEvent& e) {
  static_cast<void>(ctx);
  if (!m_active || m_activeHandle == -1) return {};

  const float cx = e.fpoint.x, cy = e.fpoint.y;

  // ── Distort モード: コーナー自由移動 ───────────────────────────────────────
  if (m_distortMode && m_distortDragCorner >= 0) {
    m_distortCorners[m_distortDragCorner] = {cx, cy};
    updateHandles();
    ToolResult r; r.viewportChanged = true; return r;
  }

  if (m_activeHandle == -2) {
    // ── 移動 ──────────────────────────────────────────────────────────────
    const float ddx = cx - m_dragAnchorX;
    const float ddy = cy - m_dragAnchorY;
    m_tx = m_dragTx0 + ddx;
    m_ty = m_dragTy0 + ddy;

    if (m_distortMode) {
      // distort モード: 全コーナーをドラッグ開始位置から delta でシフト
      for (int i = 0; i < 4; ++i) {
        m_distortCorners[i].x = m_dragDistortCorners[i].x + ddx;
        m_distortCorners[i].y = m_dragDistortCorners[i].y + ddy;
      }
    }

  } else if (m_activeHandle == 8) {
    // ── 回転 ──────────────────────────────────────────────────────────────
    const float angle = std::atan2(cy - (m_originY + m_ty),
                                    cx - (m_originX + m_tx));
    m_rot = m_dragRot0 + (angle - m_dragAngle0);

    // distort モード中の回転: コーナーを再同期
    if (m_distortMode) {
      // 一時的にアフィンコーナーで上書き（回転ハンドルは distort を解除しない）
      updateHandles();
      for (int i = 0; i < 4; ++i) m_distortCorners[i] = m_corners[i];
    }

  } else if (m_activeHandle >= 0 && m_activeHandle <= 7) {
    // ── スケール ──────────────────────────────────────────────────────────
    const float dx = cx - m_dragAnchorX;
    const float dy = cy - m_dragAnchorY;
    const float cosR = std::cos(m_rot), sinR = std::sin(m_rot);
    const float dLocalX = cosR * dx + sinR * dy;
    const float dLocalY = -sinR * dx + cosR * dy;

    static constexpr int kSxSign[8] = {-1, 0, +1, -1, +1, -1,  0, +1};
    static constexpr int kSySign[8] = {-1,-1,  -1, 0,   0, +1, +1, +1};

    const int sxS = kSxSign[m_activeHandle];
    const int syS = kSySign[m_activeHandle];

    if (sxS != 0 && m_halfW > 0.5f) {
      const float newSx = dLocalX / (2.f * static_cast<float>(sxS) * m_halfW);
      if (std::abs(newSx) > 0.01f) m_sx = newSx;
    }
    if (syS != 0 && m_halfH > 0.5f) {
      const float newSy = dLocalY / (2.f * static_cast<float>(syS) * m_halfH);
      if (std::abs(newSy) > 0.01f) m_sy = newSy;
    }

    if (sxS != 0 && syS != 0 && !e.shift) {
      const float dragAspect = (m_dragSy0 == 0.f) ? 1.f : (m_dragSx0 / m_dragSy0);
      if (std::abs(m_sx) > std::abs(m_sy) * dragAspect)
        m_sy = std::copysign(m_sx / dragAspect, m_sy);
      else
        m_sx = std::copysign(m_sy * dragAspect, m_sx);
    }

    if (sxS != 0 && syS != 0) {
      const float fsxS = static_cast<float>(sxS);
      const float fsyS = static_cast<float>(syS);
      m_tx = m_dragAnchorX - m_originX
           + cosR * fsxS * m_sx * m_halfW
           - sinR * fsyS * m_sy * m_halfH;
      m_ty = m_dragAnchorY - m_originY
           + sinR * fsxS * m_sx * m_halfW
           + cosR * fsyS * m_sy * m_halfH;
    } else {
      if (sxS != 0) {
        m_tx = (cx + m_dragAnchorX) * 0.5f - m_originX;
      } else {
        m_tx = m_dragTx0;
      }
      if (syS != 0) {
        m_ty = (cy + m_dragAnchorY) * 0.5f - m_originY;
      } else {
        m_ty = m_dragTy0;
      }
    }

    // distort モード中のスケール: コーナーを再同期
    if (m_distortMode) {
      updateHandles();
      for (int i = 0; i < 4; ++i) m_distortCorners[i] = m_corners[i];
    }
  }

  updateHandles();
  ToolResult r; r.viewportChanged = true; return r;
}

ToolResult FreeTransformTool::onPointerRelease(ToolContext& ctx, const ToolPointerEvent& e) {
  static_cast<void>(ctx); static_cast<void>(e);
  m_activeHandle      = -1;
  m_distortDragCorner = -1;
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
