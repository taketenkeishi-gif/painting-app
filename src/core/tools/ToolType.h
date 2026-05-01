#pragma once

namespace core {

enum class ToolKind {
  Brush,
  Eraser,
  Eyedropper,
  Hand,
  Zoom,
  Line,
  RectSelection,
  MoveLayer,
  Fill,
  Pen
};

const char* toolKindDisplayName(ToolKind kind) noexcept;

} // namespace core
