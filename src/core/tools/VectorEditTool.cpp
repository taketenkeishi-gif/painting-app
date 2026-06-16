#include "core/tools/VectorEditTool.h"

#include <cmath>
#include <algorithm>

#include "core/document/Document.h"
#include "core/tools/ToolContext.h"

namespace core {

// ─── helpers ────────────────────────────────────────────────────────────────

std::vector<VectorPath>* VectorEditTool::activeVectorPaths(Document& doc) {
  Layer* layer = doc.activeLayer();
  if (layer == nullptr || layer->kind() != LayerKind::Vector) {
    return nullptr;
  }
  return &layer->vectorPaths();
}

void VectorEditTool::rebuildPointCache(const std::vector<VectorPath>& paths) {
  m_pointRefs.clear();
  m_allPoints.clear();
  for (int pi = 0; pi < static_cast<int>(paths.size()); ++pi) {
    for (int vi = 0; vi < static_cast<int>(paths[pi].points.size()); ++vi) {
      m_pointRefs.push_back({pi, vi});
      m_allPoints.push_back(paths[pi].points[vi]);
    }
  }
}

int VectorEditTool::hitTest(FPoint pt) const {
  int best = -1;
  float bestDist2 = kHitRadius * kHitRadius;
  for (int i = 0; i < static_cast<int>(m_allPoints.size()); ++i) {
    const float dx = m_allPoints[i].x - pt.x;
    const float dy = m_allPoints[i].y - pt.y;
    const float d2 = dx * dx + dy * dy;
    if (d2 < bestDist2) {
      bestDist2 = d2;
      best = i;
    }
  }
  return best;
}

// ─── ITool ──────────────────────────────────────────────────────────────────

ToolResult VectorEditTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  auto* paths = activeVectorPaths(context.document);
  if (paths == nullptr) {
    return {};
  }

  rebuildPointCache(*paths);

  const int hit = hitTest(event.fpoint);

  if (hit < 0) {
    // 空クリック → 全選択解除
    m_selectedPoints.clear();
    m_dragging = false;
    m_hasBeforeSnapshot = false;
    return {};
  }

  // Shift → トグル。それ以外は単独選択してドラッグ準備。
  if (event.shift) {
    if (m_selectedPoints.count(hit)) {
      m_selectedPoints.erase(hit);
    } else {
      m_selectedPoints.insert(hit);
    }
    m_dragging = false;
    m_hasBeforeSnapshot = false;
  } else {
    if (!m_selectedPoints.count(hit)) {
      // 非選択点をクリック → 選択を置き換え
      m_selectedPoints.clear();
      m_selectedPoints.insert(hit);
    }
    // 選択済み点クリック → 既存セットを維持してドラッグ開始
    m_dragging = true;
    m_dragStart = event.fpoint;
    m_dragLast  = event.fpoint;
    m_beforePaths = *paths;
    m_hasBeforeSnapshot = true;
  }
  return {};
}

ToolResult VectorEditTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_dragging || m_selectedPoints.empty()) {
    return {};
  }

  auto* paths = activeVectorPaths(context.document);
  if (paths == nullptr) {
    return {};
  }

  const float dx = event.fpoint.x - m_dragLast.x;
  const float dy = event.fpoint.y - m_dragLast.y;
  m_dragLast = event.fpoint;

  // 選択点を移動
  for (int flatIdx : m_selectedPoints) {
    if (flatIdx < 0 || flatIdx >= static_cast<int>(m_pointRefs.size())) { continue; }
    const auto& ref = m_pointRefs[flatIdx];
    auto& path = (*paths)[ref.pathIdx];
    if (ref.pointIdx < static_cast<int>(path.points.size())) {
      path.points[ref.pointIdx].x += dx;
      path.points[ref.pointIdx].y += dy;
    }
  }

  // キャッシュを更新（移動後の位置反映）
  rebuildPointCache(*paths);

  ToolResult result;
  result.pixelsChanged = true;
  return result;
}

ToolResult VectorEditTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  (void)event;
  if (!m_dragging) {
    return {};
  }
  m_dragging = false;

  auto* paths = activeVectorPaths(context.document);
  if (paths == nullptr || !m_hasBeforeSnapshot) {
    m_hasBeforeSnapshot = false;
    return {};
  }

  // 位置が実際に変わった場合のみアンドゥ対象
  ToolResult result;
  result.pixelsChanged = true;  // AppController 側でスナップショット比較してアンドゥ登録
  return result;
}

ToolResult VectorEditTool::onCancel(ToolContext& context) {
  if (m_dragging && m_hasBeforeSnapshot) {
    auto* paths = activeVectorPaths(context.document);
    if (paths != nullptr) {
      *paths = m_beforePaths;  // 巻き戻し
    }
  }
  m_dragging = false;
  m_hasBeforeSnapshot = false;
  m_selectedPoints.clear();

  ToolResult result;
  result.pixelsChanged = true;
  return result;
}

ToolResult VectorEditTool::onWheel(ToolContext& /*context*/, int /*deltaSteps*/, const ToolPointerEvent& /*event*/) {
  return {};
}

ToolOverlayState VectorEditTool::overlay() const {
  ToolOverlayState state;
  if (m_allPoints.empty()) {
    return state;
  }

  state.hasVectorEdit = true;
  state.vectorEditPoints.reserve(m_allPoints.size());
  state.vectorEditPointPath.reserve(m_pointRefs.size());
  state.vectorEditPointSelected.reserve(m_allPoints.size());

  for (int i = 0; i < static_cast<int>(m_allPoints.size()); ++i) {
    state.vectorEditPoints.push_back(m_allPoints[i]);
    state.vectorEditPointPath.push_back(m_pointRefs[i].pathIdx);
    state.vectorEditPointSelected.push_back(m_selectedPoints.count(i) > 0);
  }

  return state;
}

bool VectorEditTool::deleteSelectedPoints(Document& doc) {
  if (m_selectedPoints.empty()) {
    return false;
  }

  auto* paths = activeVectorPaths(doc);
  if (paths == nullptr) {
    return false;
  }

  // パスごとに削除する点のインデックスを収集
  // m_pointRefs のフラットインデックスで参照
  // 逆順削除でインデックスずれを防ぐ
  struct ToDelete { int pathIdx; int pointIdx; };
  std::vector<ToDelete> toDelete;
  toDelete.reserve(m_selectedPoints.size());

  for (int flat : m_selectedPoints) {
    if (flat < 0 || flat >= static_cast<int>(m_pointRefs.size())) { continue; }
    toDelete.push_back({m_pointRefs[flat].pathIdx, m_pointRefs[flat].pointIdx});
  }

  // パスインデックス昇順、点インデックス降順でソート → 後ろから削除
  std::sort(toDelete.begin(), toDelete.end(), [](const ToDelete& a, const ToDelete& b) {
    if (a.pathIdx != b.pathIdx) { return a.pathIdx < b.pathIdx; }
    return a.pointIdx > b.pointIdx;
  });

  for (const auto& d : toDelete) {
    auto& pts = (*paths)[d.pathIdx].points;
    if (d.pointIdx >= 0 && d.pointIdx < static_cast<int>(pts.size())) {
      pts.erase(pts.begin() + d.pointIdx);
    }
  }

  // 空になったパスを削除
  paths->erase(std::remove_if(paths->begin(), paths->end(),
      [](const VectorPath& p) { return p.points.empty(); }),
      paths->end());

  m_selectedPoints.clear();
  rebuildPointCache(*paths);

  return true;
}

} // namespace core
