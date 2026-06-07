#pragma once

namespace core {

enum class ToolKind {
  Brush,
  Eraser,
  Eyedropper,
  Hand,
  Zoom,
  Shape,         ///< 図形ツール（直線・曲線・矩形・円など）
  RectSelection,
  MoveLayer,
  Fill,
  AiSelect,      ///< AI オブジェクト選択（SAM2 / スマートフラッドフィル）
  Gradient,      ///< グラデーション塗りつぶし
  FreeTransform, ///< 自由変形（Ctrl+T モーダルセッション）
};

const char* toolKindDisplayName(ToolKind kind) noexcept;

} // namespace core
