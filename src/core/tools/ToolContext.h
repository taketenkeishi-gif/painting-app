#pragma once

#include <optional>

#include "core/buffer/PixelBuffer.h"
#include "core/color/Color.h"
#include "core/common/Point.h"
#include "core/common/Rect.h"
#include "core/document/Document.h"

namespace core {

struct ToolPointerEvent {
  Point point;
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
