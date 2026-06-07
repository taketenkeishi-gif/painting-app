#pragma once

#include <optional>
#include <vector>

#include "core/buffer/PixelBuffer.h"
#include "core/color/Color.h"
#include "core/common/FPoint.h"
#include "core/common/Point.h"
#include "core/common/Rect.h"
#include "core/document/Document.h"
#include "core/selection/SelectionEngine.h"

namespace core {

struct ToolPointerEvent {
  Point point;           // 整数キャンバス座標（後方互換）
  FPoint fpoint;         // float精度キャンバス座標
  float pressure {1.0f}; // 筆圧 0.0-1.0
  float tiltX {0.0f};    // ペン傾き X (-60 to +60度)
  float tiltY {0.0f};    // ペン傾き Y
  bool isTablet {false};
  bool shift {false};
  bool ctrl {false};
  bool alt {false};
  bool isDblClick {false}; // ダブルクリック（PolygonLasso確定など）
};

enum class OverlayCursorHint {
  Default,
  Cross,         // 新規選択中
  Move,          // 選択範囲移動中
  AddSelection,  // Shift: 追加
  SubSelection,  // Alt: 減算
};

struct ToolOverlayState {
  bool hasLine {false};
  Point lineStart;
  Point lineEnd;

  bool hasRect {false};
  Rect rect;

  // Polygon / freehand path overlay (e.g. lasso selection preview)
  bool hasPolygon {false};
  bool polygonClosed {false};           // draw as closed polygon vs open path
  std::vector<Point> polygonPoints;

  // 多角形ラッソ: クリック頂点リスト + マウス追従線
  bool hasPolyLasso {false};
  std::vector<Point> polyLassoVertices;  // 確定頂点
  Point polyLassoMouse {0, 0};           // 現在マウス位置

  // ベクターストロークのライブプレビュー（描画中のみ有効）
  bool hasVectorPreview {false};
  std::vector<FPoint> vectorPreviewPoints;
  Color vectorPreviewColor {0, 0, 0, 255};
  float vectorPreviewWidth {2.0f};

  OverlayCursorHint cursorHint {OverlayCursorHint::Default};

  // FreeTransformTool 変形ボックス
  bool   hasTransformBox       {false};
  FPoint transformCorners[4]   {};   // TL TR BR BL (canvas px)
  FPoint transformHandles[9]   {};   // 0-7: スケール, 8: 回転
  int    transformActiveHandle {-1};
};

struct ToolResult {
  bool pixelsChanged {false};
  bool selectionChanged {false};
  bool viewportChanged {false};
  std::optional<Color> sampledColor;
  std::optional<Rect> dirtyRect;
};

struct ToolContext {
  Document& document;
  const PixelBuffer& composited;
  Color currentColor;
  Color secondaryColor {255, 255, 255, 255};  ///< 背景色（グラデーション用）
  int brushSize {1};
  bool maskEditMode {false};  ///< true = brush/eraser writes to layer maskBuffer (grayscale)
  SelectionEngine* selectionEngine {nullptr};  ///< 新プロバイダーパイプライン
};

} // namespace core
