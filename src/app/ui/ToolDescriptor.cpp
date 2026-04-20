#include "app/ui/ToolDescriptor.h"

namespace app::ui {

namespace {

std::vector<ToolDescriptor> buildDefaultToolCatalog() {
  return {
      ToolDescriptor {
          core::ToolKind::Brush,
          "brush",
          "Brush",
          {
              SubToolDescriptor {
                  "brush_normal",
                  "Normal",
                  BrushPreset {8, 100, 100, 100, 25, true, 35, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false},
                  {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode, ToolPropertyKey::LockAlphaRespect},
                  "LMB drag to paint. Wheel or [ ] adjusts size."},
              SubToolDescriptor {
                  "brush_hard",
                  "Hard",
                  BrushPreset {6, 100, 100, 100, 18, false, 20, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false},
                  {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode, ToolPropertyKey::LockAlphaRespect},
                  "Crisp edge stroke with tighter spacing."},
              SubToolDescriptor {
                  "brush_soft",
                  "Soft",
                  BrushPreset {12, 65, 35, 80, 35, true, 50, true, true, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false},
                  {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode, ToolPropertyKey::LockAlphaRespect},
                  "Soft edge brush for blending."},
              SubToolDescriptor {
                  "brush_airbrush",
                  "Airbrush",
                  BrushPreset {24, 28, 10, 35, 12, true, 60, true, true, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false},
                  {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode, ToolPropertyKey::LockAlphaRespect},
                  "Low-flow brush for gradual buildup."}},
          {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode, ToolPropertyKey::LockAlphaRespect},
          "Draw on active layer."},
      ToolDescriptor {
          core::ToolKind::Eraser,
          "eraser",
          "Eraser",
          {
              SubToolDescriptor {
                  "eraser_normal",
                  "Normal",
                  BrushPreset {16, 100, 100, 100, 25, true, 35, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false},
                  {ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::EraseMode},
                  "Erase with hard edge."},
              SubToolDescriptor {
                  "eraser_soft",
                  "Soft",
                  BrushPreset {22, 55, 25, 70, 28, true, 55, true, true, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false},
                  {ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::EraseMode},
                  "Erase softly with feathered edge."}},
          {ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::EraseMode},
          "Erase pixels on active layer."},
      ToolDescriptor {
          core::ToolKind::Eyedropper,
          "eyedropper",
          "Eyedropper",
          {SubToolDescriptor {"eyedropper_default", "Sample Composite", {}, {}, "Click canvas to sample color."}},
          {ToolPropertyKey::Color},
          "Pick a color from composited result."},
      ToolDescriptor {
          core::ToolKind::Fill,
          "fill",
          "Fill",
          {SubToolDescriptor {"fill_default", "Contiguous Fill", {}, {}, "Click to fill connected area."}},
          {},
          "Fill connected pixels."},
      ToolDescriptor {
          core::ToolKind::Line,
          "line",
          "Line",
          {SubToolDescriptor {"line_default", "Straight Line", BrushPreset {8, 100, 100, 100, 25, true, 10, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false}, {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::ShapeType, ToolPropertyKey::AntiAlias, ToolPropertyKey::BlendMode}, "Drag to draw straight line."}},
          {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::ShapeType, ToolPropertyKey::AntiAlias, ToolPropertyKey::BlendMode},
          "Draw straight lines."},
      ToolDescriptor {
          core::ToolKind::RectSelection,
          "rect_selection",
          "Rect Selection",
          {SubToolDescriptor {"rect_default", "Rectangle", {}, {}, "Drag to create selection."}},
          {},
          "Create rectangular selection."},
      ToolDescriptor {
          core::ToolKind::MoveLayer,
          "move_layer",
          "Move Layer",
          {SubToolDescriptor {"move_layer_default", "Pixel Offset", {}, {}, "Drag to offset active layer."}},
          {},
          "Move active layer pixels."},
      ToolDescriptor {
          core::ToolKind::Hand,
          "hand",
          "Hand",
          {SubToolDescriptor {"hand_default", "Pan View", {}, {}, "Drag to pan viewport."}},
          {},
          "Pan viewport."},
      ToolDescriptor {
          core::ToolKind::Zoom,
          "zoom",
          "Zoom",
          {SubToolDescriptor {"zoom_default", "Wheel Zoom", {}, {}, "Use Ctrl+Wheel to zoom."}},
          {},
          "Zoom viewport."}};
}

} // namespace

ToolCatalog::ToolCatalog()
    : m_tools(buildDefaultToolCatalog()) {}

const ToolDescriptor* ToolCatalog::findTool(core::ToolKind kind) const noexcept {
  for (const ToolDescriptor& tool : m_tools) {
    if (tool.kind == kind) {
      return &tool;
    }
  }
  return nullptr;
}

const SubToolDescriptor* ToolCatalog::findSubTool(core::ToolKind kind, std::string_view subToolId) const noexcept {
  const ToolDescriptor* tool = findTool(kind);
  if (tool == nullptr) {
    return nullptr;
  }

  for (const SubToolDescriptor& sub : tool->subTools) {
    if (sub.id == subToolId) {
      return &sub;
    }
  }
  return nullptr;
}

const SubToolDescriptor* ToolCatalog::defaultSubTool(core::ToolKind kind) const noexcept {
  const ToolDescriptor* tool = findTool(kind);
  if (tool == nullptr || tool->subTools.empty()) {
    return nullptr;
  }
  return &tool->subTools.front();
}

} // namespace app::ui
