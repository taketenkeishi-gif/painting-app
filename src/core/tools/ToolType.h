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
  AiSelect,   ///< AI オブジェクト選択（SAM2 / スマートフラッドフィル）
};

const char* toolKindDisplayName(ToolKind kind) noexcept;

} // namespace core
