#pragma once

#include <optional>
#include <vector>

#include "core/buffer/PixelBuffer.h"
#include "core/color/Color.h"
#include "core/common/FPoint.h"
#include "core/common/Point.h"
#include "core/common/Rect.h"
#include "core/document/Document.h"

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
  int brushSize {1};
};

} // namespace core
