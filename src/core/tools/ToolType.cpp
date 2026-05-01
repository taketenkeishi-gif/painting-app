#include "core/tools/ToolType.h"

namespace core {

const char* toolKindDisplayName(ToolKind kind) noexcept {
  switch (kind) {
    case ToolKind::Brush:
      return "Brush";
    case ToolKind::Pen:
      return "Pen";
    case ToolKind::Eraser:
      return "Eraser";
    case ToolKind::Eyedropper:
      return "Eyedropper";
    case ToolKind::Hand:
      return "Hand";
    case ToolKind::Zoom:
      return "Zoom";
    case ToolKind::Line:
      return "Line";
    case ToolKind::RectSelection:
      return "Rect Selection";
    case ToolKind::MoveLayer:
      return "Move Layer";
    case ToolKind::Fill:
      return "Fill";
    default:
      return "Unknown";
  }
}

} // namespace core
