#include "core/tools/MoveLayerTool.h"

#include "core/selection/SelectionMask.h"

namespace core {

ToolResult MoveLayerTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  m_dragging = true;
  m_start    = event.point;
  m_current  = event.point;

  // ラスターレイヤーは現在のオフセットを記憶する。
  // ドラッグ中はこのベース値に delta を足し続けることでライブ移動を実現する。
  const Layer* active = context.document.activeLayer();
  if (active != nullptr && active->kind() == LayerKind::Raster) {
    m_baseOffsetX = active->offsetX();
    m_baseOffsetY = active->offsetY();
  } else {
    m_baseOffsetX = m_baseOffsetY = 0;
  }

  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult MoveLayerTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_dragging) {
    return {};
  }
  m_current = event.point;

  Layer* active = context.document.activeLayer();
  if (active == nullptr || active->locked() || active->positionLocked()) {
    ToolResult r;
    r.viewportChanged = true;
    return r;
  }

  // ラスターレイヤーのみオフセット移動（ライブプレビュー）。
  // ベクターレイヤーは release 時にまとめて移動するため move では変更しない。
  if (active->kind() == LayerKind::Raster) {
    const int dx = m_current.x - m_start.x;
    const int dy = m_current.y - m_start.y;
    active->setOffset(m_baseOffsetX + dx, m_baseOffsetY + dy);

    ToolResult result;
    result.pixelsChanged  = true;   // rerender + canvasChanged を要求
    result.viewportChanged = true;
    return result;
  }

  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult MoveLayerTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_dragging) {
    return {};
  }

  m_dragging = false;
  m_current  = event.point;

  Layer* active = context.document.activeLayer();
  if (active == nullptr) {
    return {};
  }
  if (active->locked() || active->positionLocked()) {
    ToolResult result;
    result.viewportChanged = true;
    return result;
  }

  const int dx = m_current.x - m_start.x;
  const int dy = m_current.y - m_start.y;
  if (dx == 0 && dy == 0) {
    ToolResult result;
    result.viewportChanged = true;
    return result;
  }

  // ── ベクターレイヤー ─────────────────────────────────────────────────────
  // ベクターパスはキャンバス絶対座標を持つため、従来通り点座標を平行移動する。
  if (active->kind() == LayerKind::Vector) {
    active->moveVectorPathsBy(dx, dy);
    ToolResult result;
    result.pixelsChanged  = true;
    result.viewportChanged = true;
    result.dirtyRect = Rect {0, 0, active->buffer().width(), active->buffer().height()};
    return result;
  }

  // ── ラスターレイヤー ─────────────────────────────────────────────────────
  // move 時点で既にオフセットは更新済み（ライブプレビュー）。
  // release では確定フラグを返すだけ。ピクセルデータは一切変更しない。
  if (active->kind() == LayerKind::Raster) {
    // 選択範囲がある場合はそれも追従させる。
    const SelectionMask& selection = context.document.selection();
    if (selection.hasSelection()) {
      context.document.selection().translate(dx, dy);
    }

    ToolResult result;
    result.pixelsChanged   = true;   // undo 履歴への書き込みを許可
    result.selectionChanged = selection.hasSelection();
    result.viewportChanged  = true;
    result.dirtyRect = Rect {0, 0,
        context.document.canvasSize().width,
        context.document.canvasSize().height};
    return result;
  }

  // その他のレイヤー種別（Adjustment / Folder / Text）: 移動なし
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult MoveLayerTool::onCancel(ToolContext& context) {
  if (!m_dragging) {
    return {};
  }
  m_dragging = false;

  // ラスターレイヤーのドラッグ中にキャンセルが発生した場合、
  // ドラッグ開始時点のオフセットに戻す。
  Layer* active = context.document.activeLayer();
  if (active != nullptr && active->kind() == LayerKind::Raster) {
    active->setOffset(m_baseOffsetX, m_baseOffsetY);
    ToolResult result;
    result.pixelsChanged  = true;
    result.viewportChanged = true;
    return result;
  }

  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult MoveLayerTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

ToolOverlayState MoveLayerTool::overlay() const {
  return {};
}

} // namespace core
