#include "app/ui/ToolDescriptor.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace app::ui {

namespace {

ToolBehaviorProfile makeProfile(const BrushPreset& preset) {
  ToolBehaviorProfile profile;
  profile.stroke.size = preset.size;
  profile.stroke.opacity = preset.opacity;
  profile.stroke.flow = preset.flow;
  profile.stroke.spacing = preset.spacing;
  profile.stroke.antiAlias = preset.antiAlias;
  profile.shape.shapeType = preset.shapeType;
  profile.shape.hardness = preset.hardness;
  profile.shape.angle = preset.angle;
  profile.shape.roundness = preset.roundness;
  profile.shape.taperStart = preset.taperStart;
  profile.shape.taperEnd = preset.taperEnd;
  profile.stabilizer.stabilization = preset.stabilization;
  profile.stabilizer.postCorrection = preset.postCorrection;
  profile.stabilizer.velocityBasedCorrection = preset.velocityBasedCorrection;
  profile.vector.strokeWidth = preset.strokeWidth > 0 ? preset.strokeWidth : preset.size;
  profile.vector.snapAngle = preset.snapAngle;
  profile.vector.simplifyLevel = preset.simplifyLevel;
  profile.fill.threshold = preset.fillThreshold;
  profile.fill.contiguous = preset.fillContiguous;
  profile.fill.referAllLayers = preset.fillReferAllLayers;
  profile.fill.gapClose = preset.fillGapClose;
  profile.selection.mode = preset.selectionMode;
  profile.selection.autoSelectThreshold = preset.autoSelectThreshold;
  profile.selection.autoSelectContiguous = preset.autoSelectContiguous;
  profile.selection.autoSelectReferAllLayers = preset.autoSelectReferAllLayers;
  profile.selection.featherRadius = preset.selectionFeather;
  profile.selection.antiAlias = preset.selectionAntiAlias;
  profile.blendMode = preset.blendMode;
  profile.eraseMode = preset.eraseMode;
  profile.lockAlphaRespect = preset.lockAlphaRespect;
  profile.vectorEraseMode = preset.vectorEraseMode;
  profile.vectorTrimOutside = preset.vectorTrimOutside;
  profile.targetLayerKind = preset.targetLayerKind;
  profile.cursorStyle = preset.cursorStyle;
  return profile;
}

SubToolDescriptor makeSubTool(
    std::string id,
    std::string displayName,
    BrushPreset preset,
    std::vector<ToolPropertyKey> editable,
    std::string guide) {
  SubToolDescriptor descriptor;
  descriptor.id = std::move(id);
  descriptor.displayName = std::move(displayName);
  descriptor.preset = preset;
  descriptor.profile = makeProfile(preset);
  descriptor.editableProperties = std::move(editable);
  descriptor.guide = std::move(guide);
  return descriptor;
}

std::vector<ToolDescriptor> buildDefaultToolCatalog() {
  return {
      ToolDescriptor {
          core::ToolKind::Brush,
          "brush",
          "Brush",
          {
              makeSubTool(
                  "brush_normal",
                  "Normal",
                  BrushPreset {8, 100, 100, 100, 25, true, 35, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Brush},
                  {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::Angle, ToolPropertyKey::Roundness, ToolPropertyKey::TaperStart, ToolPropertyKey::TaperEnd, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode, ToolPropertyKey::LockAlphaRespect},
                  "LMB drag to paint. Wheel or [ ] adjusts size."),
              makeSubTool(
                  "brush_hard",
                  "Hard",
                  BrushPreset {6, 100, 100, 100, 18, false, 20, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Brush},
                  {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::Angle, ToolPropertyKey::Roundness, ToolPropertyKey::TaperStart, ToolPropertyKey::TaperEnd, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode, ToolPropertyKey::LockAlphaRespect},
                  "Crisp edge stroke with tighter spacing."),
              makeSubTool(
                  "brush_soft",
                  "Soft",
                  BrushPreset {12, 65, 35, 80, 35, true, 50, true, true, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Brush},
                  {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::Angle, ToolPropertyKey::Roundness, ToolPropertyKey::TaperStart, ToolPropertyKey::TaperEnd, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode, ToolPropertyKey::LockAlphaRespect},
                  "Soft edge brush for blending."),
              makeSubTool(
                  "brush_airbrush",
                  "Airbrush",
                  BrushPreset {24, 28, 10, 35, 12, true, 60, true, true, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Brush},
                  {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::Angle, ToolPropertyKey::Roundness, ToolPropertyKey::TaperStart, ToolPropertyKey::TaperEnd, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode, ToolPropertyKey::LockAlphaRespect},
                  "Low-flow brush for gradual buildup."),
              // ── ベクターレイヤー用ブラシ ──────────────────────────────────
              makeSubTool(
                  "brush_vector_pen",
                  "Gペン (Vector)",
                  BrushPreset {4, 100, 100, 100, 20, true, 30, false, true, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Vector, CursorStyle::Brush},
                  {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection},
                  "ベクターレイヤーにGペン風のストロークを描きます。"),
              makeSubTool(
                  "brush_vector_round",
                  "丸ペン (Vector)",
                  BrushPreset {2, 100, 100, 100, 15, true, 20, false, true, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Vector, CursorStyle::Brush},
                  {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection},
                  "ベクターレイヤーに細い丸ペンのストロークを描きます。"),
              makeSubTool(
                  "brush_vector_soft",
                  "ソフト (Vector)",
                  BrushPreset {8, 80, 100, 60, 25, true, 50, true, true, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Vector, CursorStyle::Brush},
                  {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection},
                  "ベクターレイヤーにソフトなストロークを描きます。")},
          {ToolPropertyKey::Color, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::Angle, ToolPropertyKey::Roundness, ToolPropertyKey::TaperStart, ToolPropertyKey::TaperEnd, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode, ToolPropertyKey::LockAlphaRespect},
          "Draw on active layer."},
      ToolDescriptor {
          core::ToolKind::Eraser,
          "eraser",
          "Eraser",
          {
              makeSubTool(
                  "eraser_normal",
                  "Normal",
                  BrushPreset {16, 100, 100, 100, 25, true, 35, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, TargetLayerKind::Raster, CursorStyle::Brush},
                  {ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::EraseMode},
                  "Erase with hard edge."),
              makeSubTool(
                  "eraser_soft",
                  "Soft",
                  BrushPreset {22, 55, 25, 70, 28, true, 55, true, true, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, TargetLayerKind::Raster, CursorStyle::Brush},
                  {ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::EraseMode},
                  "Erase softly with feathered edge."),
              makeSubTool(
                  "eraser_vector_touch",
                  "Vector Erase (Touched)",
                  []() {
                    BrushPreset p {18, 100, 100, 100, 30, true, 30, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, TargetLayerKind::Vector, CursorStyle::Cross};
                    p.vectorEraseMode = VectorEraserMode::TouchedOnly;
                    p.vectorTrimOutside = false;
                    return p;
                  }(),
                  {ToolPropertyKey::Size, ToolPropertyKey::Spacing, ToolPropertyKey::Stabilization, ToolPropertyKey::VectorEraseMode, ToolPropertyKey::VectorTrimOutside},
                  "Erase touched part of vector strokes."),
              makeSubTool(
                  "eraser_vector_intersection",
                  "Vector Erase (To Intersection)",
                  []() {
                    BrushPreset p {18, 100, 100, 100, 30, true, 30, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, TargetLayerKind::Vector, CursorStyle::Cross};
                    p.vectorEraseMode = VectorEraserMode::ToIntersection;
                    p.vectorTrimOutside = false;
                    return p;
                  }(),
                  {ToolPropertyKey::Size, ToolPropertyKey::Spacing, ToolPropertyKey::Stabilization, ToolPropertyKey::VectorEraseMode, ToolPropertyKey::VectorTrimOutside},
                  "Erase from touched point to nearest intersections."),
              makeSubTool(
                  "eraser_vector_trim",
                  "Vector Erase (Trim Outside)",
                  []() {
                    BrushPreset p {18, 100, 100, 100, 30, true, 30, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, TargetLayerKind::Vector, CursorStyle::Cross};
                    p.vectorEraseMode = VectorEraserMode::TrimOutside;
                    p.vectorTrimOutside = true;
                    return p;
                  }(),
                  {ToolPropertyKey::Size, ToolPropertyKey::Spacing, ToolPropertyKey::Stabilization, ToolPropertyKey::VectorEraseMode, ToolPropertyKey::VectorTrimOutside},
                  "Trim protruding vector segments."),
              makeSubTool(
                  "eraser_vector_whole",
                  "Vector Erase Whole",
                  []() {
                    BrushPreset p {18, 100, 100, 100, 30, true, 30, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, TargetLayerKind::Vector, CursorStyle::Cross};
                    p.vectorEraseMode = VectorEraserMode::TouchedOnly;
                    p.vectorTrimOutside = false;
                    return p;
                  }(),
                  {ToolPropertyKey::Size, ToolPropertyKey::Spacing, ToolPropertyKey::Stabilization, ToolPropertyKey::VectorEraseMode, ToolPropertyKey::VectorTrimOutside},
                  "Legacy vector eraser preset.")},
          {ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::Flow, ToolPropertyKey::Spacing, ToolPropertyKey::AntiAlias, ToolPropertyKey::Stabilization, ToolPropertyKey::PostCorrection, ToolPropertyKey::VelocityCorrection, ToolPropertyKey::ShapeType, ToolPropertyKey::EraseMode},
          "Erase pixels on active layer."},
      ToolDescriptor {
          core::ToolKind::Eyedropper,
          "eyedropper",
          "Eyedropper",
          {makeSubTool("eyedropper_default", "Sample Composite", BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Cross}, {}, "Click canvas to sample color.")},
          {ToolPropertyKey::Color},
          "Pick a color from composited result."},
      ToolDescriptor {
          core::ToolKind::Fill,
          "fill",
          "塗りつぶし",
          {
              makeSubTool(
                  "fill_contiguous",
                  "塗りつぶし",
                  []() {
                    BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Raster, CursorStyle::Fill};
                    p.fillThreshold = 16;
                    p.fillContiguous = true;
                    p.fillReferAllLayers = false;
                    p.fillGapClose = 0;
                    return p;
                  }(),
                  {ToolPropertyKey::FillThreshold, ToolPropertyKey::FillContiguous, ToolPropertyKey::FillReferAllLayers, ToolPropertyKey::FillGapClose, ToolPropertyKey::EraseMode},
                  "クリックで隣接領域を塗りつぶし。許容範囲でしきい値調整。"),
              makeSubTool(
                  "fill_all",
                  "全体塗りつぶし",
                  []() {
                    BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Raster, CursorStyle::Fill};
                    p.fillThreshold = 16;
                    p.fillContiguous = false;
                    p.fillReferAllLayers = false;
                    p.fillGapClose = 0;
                    return p;
                  }(),
                  {ToolPropertyKey::FillThreshold, ToolPropertyKey::FillContiguous, ToolPropertyKey::FillReferAllLayers, ToolPropertyKey::FillGapClose, ToolPropertyKey::EraseMode},
                  "クリックでキャンバス内の同色ピクセルをすべて塗りつぶし。"),
              makeSubTool(
                  "fill_gapclose",
                  "隙間閉じ",
                  []() {
                    BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Raster, CursorStyle::Fill};
                    p.fillThreshold = 24;
                    p.fillContiguous = true;
                    p.fillReferAllLayers = true;
                    p.fillGapClose = 3;
                    return p;
                  }(),
                  {ToolPropertyKey::FillThreshold, ToolPropertyKey::FillContiguous, ToolPropertyKey::FillReferAllLayers, ToolPropertyKey::FillGapClose, ToolPropertyKey::EraseMode},
                  "線画の隙間を自動補完して塗りつぶし。全レイヤー参照。"),
              makeSubTool(
                  "fill_erase",
                  "消去塗りつぶし",
                  []() {
                    BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, TargetLayerKind::Raster, CursorStyle::Fill};
                    p.fillThreshold = 0;
                    p.fillContiguous = true;
                    p.fillReferAllLayers = false;
                    p.fillGapClose = 0;
                    p.eraseMode = true;
                    return p;
                  }(),
                  {ToolPropertyKey::FillContiguous, ToolPropertyKey::FillReferAllLayers, ToolPropertyKey::EraseMode},
                  "クリックで隣接する有色ピクセルを透明化（消去）。")},
          {ToolPropertyKey::FillThreshold, ToolPropertyKey::FillContiguous, ToolPropertyKey::FillReferAllLayers, ToolPropertyKey::FillGapClose, ToolPropertyKey::EraseMode},
          "クリックでピクセルを塗りつぶし。"},
      ToolDescriptor {
          core::ToolKind::Line,
          "line",
          "Line",
          {
              makeSubTool(
                  "line_raster",
                  "Raster Line",
                  BrushPreset {8, 100, 100, 100, 25, true, 10, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Raster, CursorStyle::Cross, 0, 0, 8},
                  {ToolPropertyKey::Color, ToolPropertyKey::StrokeWidth, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::ShapeType, ToolPropertyKey::AntiAlias, ToolPropertyKey::BlendMode, ToolPropertyKey::Angle},
                  "Drag to draw straight raster line."),
              makeSubTool(
                  "line_vector",
                  "Vector Basic",
                  BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Vector, CursorStyle::Cross, 0, 0, 8},
                  {ToolPropertyKey::Color, ToolPropertyKey::StrokeWidth, ToolPropertyKey::Opacity, ToolPropertyKey::AntiAlias, ToolPropertyKey::SnapAngle, ToolPropertyKey::SimplifyLevel},
                  "Drag to create vector line path."),
              makeSubTool(
                  "line_vector_snap",
                  "Vector Snap",
                  BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Vector, CursorStyle::Cross, 15, 0, 8},
                  {ToolPropertyKey::Color, ToolPropertyKey::StrokeWidth, ToolPropertyKey::Opacity, ToolPropertyKey::AntiAlias, ToolPropertyKey::SnapAngle, ToolPropertyKey::SimplifyLevel},
                  "Drag to create snapped vector line."),
              makeSubTool(
                  "line_vector_thick",
                  "Vector Thick",
                  BrushPreset {16, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Vector, CursorStyle::Cross, 0, 0, 16},
                  {ToolPropertyKey::Color, ToolPropertyKey::StrokeWidth, ToolPropertyKey::Opacity, ToolPropertyKey::AntiAlias, ToolPropertyKey::SnapAngle, ToolPropertyKey::SimplifyLevel},
                  "Drag to create thick vector line."),
              makeSubTool(
                  "line_vector_thin",
                  "Vector Thin",
                  BrushPreset {3, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Vector, CursorStyle::Cross, 0, 0, 3},
                  {ToolPropertyKey::Color, ToolPropertyKey::StrokeWidth, ToolPropertyKey::Opacity, ToolPropertyKey::AntiAlias, ToolPropertyKey::SnapAngle, ToolPropertyKey::SimplifyLevel},
                  "Drag to create thin vector line.")},
          {ToolPropertyKey::Color, ToolPropertyKey::StrokeWidth, ToolPropertyKey::Size, ToolPropertyKey::Opacity, ToolPropertyKey::Hardness, ToolPropertyKey::ShapeType, ToolPropertyKey::AntiAlias, ToolPropertyKey::BlendMode, ToolPropertyKey::SnapAngle, ToolPropertyKey::SimplifyLevel, ToolPropertyKey::Angle},
          "Draw straight lines."},
      ToolDescriptor {
          core::ToolKind::RectSelection,
          "rect_selection",
          "選択",
          {
              makeSubTool(
                  "rect_default",
                  "矩形選択",
                  []() {
                    BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Cross};
                    p.selectionMode = SelectionMode::Rectangle;
                    p.selectionAntiAlias = true;
                    return p;
                  }(),
                  {ToolPropertyKey::SelectionMode, ToolPropertyKey::SelectionOp, ToolPropertyKey::SelectionFeather, ToolPropertyKey::SelectionAntiAlias, ToolPropertyKey::SelectionExpand, ToolPropertyKey::SelectionGapClose},
                  "ドラッグして矩形選択。Shift=正方形制約。選択範囲内ドラッグで移動。"),
              makeSubTool(
                  "lasso_default",
                  "投げ縄",
                  []() {
                    BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Cross};
                    p.selectionMode = SelectionMode::Lasso;
                    p.selectionAntiAlias = true;
                    return p;
                  }(),
                  {ToolPropertyKey::SelectionMode, ToolPropertyKey::SelectionOp, ToolPropertyKey::SelectionFeather, ToolPropertyKey::SelectionAntiAlias, ToolPropertyKey::SelectionExpand, ToolPropertyKey::SelectionGapClose},
                  "フリーハンドで任意形状を選択。選択範囲内ドラッグで移動。"),
              makeSubTool(
                  "poly_lasso",
                  "多角形選択",
                  []() {
                    BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Cross};
                    p.selectionMode = SelectionMode::PolygonLasso;
                    p.selectionAntiAlias = true;
                    return p;
                  }(),
                  {ToolPropertyKey::SelectionMode, ToolPropertyKey::SelectionOp, ToolPropertyKey::SelectionFeather, ToolPropertyKey::SelectionAntiAlias, ToolPropertyKey::SelectionExpand, ToolPropertyKey::SelectionGapClose},
                  "クリックで頂点を追加。始点付近クリックまたはダブルクリックで確定。Escでキャンセル。"),
              makeSubTool(
                  "auto_select",
                  "自動選択",
                  []() {
                    BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Cross};
                    p.selectionMode = SelectionMode::AutoSelect;
                    p.autoSelectThreshold = 20;
                    p.autoSelectContiguous = true;
                    p.autoSelectReferAllLayers = true;
                    return p;
                  }(),
                  {ToolPropertyKey::SelectionOp, ToolPropertyKey::AutoSelectThreshold, ToolPropertyKey::AutoSelectContiguous, ToolPropertyKey::AutoSelectReferAllLayers, ToolPropertyKey::SelectionFeather, ToolPropertyKey::SelectionAntiAlias, ToolPropertyKey::SelectionEdgeSnap, ToolPropertyKey::SelectionGapClose},
                  "クリック点と類似した連続した色域を選択。Shift=追加、Alt=減算。"),
              makeSubTool(
                  "object_select",
                  "オブジェクト選択",
                  []() {
                    BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Cross};
                    p.selectionMode = SelectionMode::ObjectSelect;
                    p.autoSelectThreshold = 24;
                    p.autoSelectContiguous = false;
                    p.autoSelectReferAllLayers = true;
                    return p;
                  }(),
                  {ToolPropertyKey::SelectionOp, ToolPropertyKey::AutoSelectThreshold, ToolPropertyKey::AutoSelectReferAllLayers, ToolPropertyKey::SelectionFeather, ToolPropertyKey::SelectionAntiAlias, ToolPropertyKey::SelectionEdgeSnap, ToolPropertyKey::SelectionGapClose},
                  "クリックで全レイヤーの類似色をまとめて選択。ComfyUI 接続時は SAM2 で高精度化。Shift=追加、Alt=減算。"),
          },
          {ToolPropertyKey::AutoSelectThreshold, ToolPropertyKey::AutoSelectContiguous, ToolPropertyKey::AutoSelectReferAllLayers},
          "選択範囲を作成します。"},
      ToolDescriptor {
          core::ToolKind::MoveLayer,
          "move_layer",
          "Move Layer",
          {makeSubTool("move_layer_default", "Pixel/Vector Offset", BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Hand}, {}, "Drag to offset active layer content.")},
          {},
          "Move active layer pixels or vector paths."},
      ToolDescriptor {
          core::ToolKind::Hand,
          "hand",
          "Hand",
          {makeSubTool("hand_default", "Pan View", BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Hand}, {}, "Drag to pan viewport.")},
          {},
          "Pan viewport."},
      ToolDescriptor {
          core::ToolKind::Zoom,
          "zoom",
          "Zoom",
          {makeSubTool("zoom_default", "Wheel Zoom", BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, TargetLayerKind::Both, CursorStyle::Zoom}, {}, "Use Ctrl+Wheel to zoom.")},
          {},
          "Zoom viewport."},
      ToolDescriptor {
          core::ToolKind::Gradient,
          "gradient",
          "グラデーション",
          {
              makeSubTool(
                  "gradient_linear_fg_bg",
                  "直線 (描画色→背景色)",
                  []() {
                    BrushPreset p {};
                    p.opacity       = 100;
                    p.blendMode     = core::BlendMode::Normal;
                    p.eraseMode     = false;
                    p.gradientType  = 0;  // Linear
                    p.gradientFill  = 0;  // FgToBg
                    p.targetLayerKind = TargetLayerKind::Raster;
                    p.cursorStyle   = CursorStyle::Cross;
                    return p;
                  }(),
                  {ToolPropertyKey::Opacity, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode},
                  "ドラッグで直線グラデーションを描画色→背景色で適用。"),
              makeSubTool(
                  "gradient_linear_fg_trans",
                  "直線 (描画色→透明)",
                  []() {
                    BrushPreset p {};
                    p.opacity       = 100;
                    p.blendMode     = core::BlendMode::Normal;
                    p.eraseMode     = false;
                    p.gradientType  = 0;  // Linear
                    p.gradientFill  = 1;  // FgToTransparent
                    p.targetLayerKind = TargetLayerKind::Raster;
                    p.cursorStyle   = CursorStyle::Cross;
                    return p;
                  }(),
                  {ToolPropertyKey::Opacity, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode},
                  "ドラッグで直線グラデーションを描画色→透明で適用。"),
              makeSubTool(
                  "gradient_radial_fg_bg",
                  "放射 (描画色→背景色)",
                  []() {
                    BrushPreset p {};
                    p.opacity       = 100;
                    p.blendMode     = core::BlendMode::Normal;
                    p.eraseMode     = false;
                    p.gradientType  = 1;  // Radial
                    p.gradientFill  = 0;  // FgToBg
                    p.targetLayerKind = TargetLayerKind::Raster;
                    p.cursorStyle   = CursorStyle::Cross;
                    return p;
                  }(),
                  {ToolPropertyKey::Opacity, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode},
                  "ドラッグで放射グラデーションを描画色→背景色で適用。"),
              makeSubTool(
                  "gradient_radial_fg_trans",
                  "放射 (描画色→透明)",
                  []() {
                    BrushPreset p {};
                    p.opacity       = 100;
                    p.blendMode     = core::BlendMode::Normal;
                    p.eraseMode     = false;
                    p.gradientType  = 1;  // Radial
                    p.gradientFill  = 1;  // FgToTransparent
                    p.targetLayerKind = TargetLayerKind::Raster;
                    p.cursorStyle   = CursorStyle::Cross;
                    return p;
                  }(),
                  {ToolPropertyKey::Opacity, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode},
                  "ドラッグで放射グラデーションを描画色→透明で適用。")},
          {ToolPropertyKey::Opacity, ToolPropertyKey::BlendMode, ToolPropertyKey::EraseMode},
          "ドラッグでグラデーションをアクティブレイヤーに適用。"}};

}

std::string normalizeName(std::string name) {
  if (name.empty()) {
    return name;
  }
  name.erase(name.begin(), std::find_if(name.begin(), name.end(), [](unsigned char c) { return !std::isspace(c); }));
  name.erase(std::find_if(name.rbegin(), name.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), name.end());
  return name;
}

} // namespace

ToolCatalog::ToolCatalog()
    : m_defaultTools(buildDefaultToolCatalog()),
      m_tools(m_defaultTools) {}

const ToolDescriptor* ToolCatalog::findTool(core::ToolKind kind) const noexcept {
  for (const ToolDescriptor& tool : m_tools) {
    if (tool.kind == kind) {
      return &tool;
    }
  }
  return nullptr;
}

ToolDescriptor* ToolCatalog::findToolMutable(core::ToolKind kind) noexcept {
  for (ToolDescriptor& tool : m_tools) {
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

SubToolDescriptor* ToolCatalog::findSubToolMutable(core::ToolKind kind, std::string_view subToolId) noexcept {
  ToolDescriptor* tool = findToolMutable(kind);
  if (tool == nullptr) {
    return nullptr;
  }

  for (SubToolDescriptor& sub : tool->subTools) {
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

bool ToolCatalog::duplicateSubTool(core::ToolKind kind, std::string_view sourceSubToolId, const std::string& newDisplayName) {
  ToolDescriptor* tool = findToolMutable(kind);
  if (tool == nullptr) {
    return false;
  }

  const auto it = std::find_if(tool->subTools.begin(), tool->subTools.end(), [&](const SubToolDescriptor& sub) {
    return sub.id == sourceSubToolId;
  });
  if (it == tool->subTools.end()) {
    return false;
  }

  SubToolDescriptor duplicated = *it;
  duplicated.id = makeSubToolId(duplicated.id, tool->subTools);
  const std::string normalized = normalizeName(newDisplayName);
  duplicated.displayName = normalized.empty() ? (duplicated.displayName + " Copy") : normalized;
  tool->subTools.push_back(std::move(duplicated));
  return true;
}

bool ToolCatalog::createSubTool(core::ToolKind kind, const std::string& newDisplayName) {
  ToolDescriptor* tool = findToolMutable(kind);
  if (tool == nullptr || tool->subTools.empty()) {
    return false;
  }
  SubToolDescriptor created = tool->subTools.front();
  created.id = makeSubToolId(tool->id + "_custom", tool->subTools);
  const std::string normalized = normalizeName(newDisplayName);
  created.displayName = normalized.empty() ? "New Sub Tool" : normalized;
  tool->subTools.push_back(std::move(created));
  return true;
}

bool ToolCatalog::renameSubTool(core::ToolKind kind, std::string_view subToolId, const std::string& newDisplayName) {
  SubToolDescriptor* sub = findSubToolMutable(kind, subToolId);
  if (sub == nullptr) {
    return false;
  }
  const std::string normalized = normalizeName(newDisplayName);
  if (normalized.empty()) {
    return false;
  }
  sub->displayName = normalized;
  return true;
}

bool ToolCatalog::removeSubTool(core::ToolKind kind, std::string_view subToolId) {
  ToolDescriptor* tool = findToolMutable(kind);
  if (tool == nullptr || tool->subTools.size() <= 1) {
    return false;
  }

  const auto it = std::find_if(tool->subTools.begin(), tool->subTools.end(), [&](const SubToolDescriptor& sub) {
    return sub.id == subToolId;
  });
  if (it == tool->subTools.end()) {
    return false;
  }
  tool->subTools.erase(it);
  return true;
}

bool ToolCatalog::resetSubTool(core::ToolKind kind, std::string_view subToolId) {
  ToolDescriptor* tool = findToolMutable(kind);
  if (tool == nullptr) {
    return false;
  }
  const auto defaultToolIt = std::find_if(m_defaultTools.begin(), m_defaultTools.end(), [&](const ToolDescriptor& entry) {
    return entry.kind == kind;
  });
  if (defaultToolIt == m_defaultTools.end()) {
    return false;
  }

  const auto defaultSubIt = std::find_if(defaultToolIt->subTools.begin(), defaultToolIt->subTools.end(), [&](const SubToolDescriptor& sub) {
    return sub.id == subToolId;
  });
  if (defaultSubIt == defaultToolIt->subTools.end()) {
    return false;
  }

  SubToolDescriptor* current = findSubToolMutable(kind, subToolId);
  if (current == nullptr) {
    return false;
  }
  *current = *defaultSubIt;
  return true;
}

std::string ToolCatalog::makeSubToolId(std::string_view baseId, const std::vector<SubToolDescriptor>& existing) {
  std::string candidate = std::string(baseId) + "_copy";
  int suffix = 1;
  auto exists = [&](const std::string& id) {
    return std::any_of(existing.begin(), existing.end(), [&](const SubToolDescriptor& sub) {
      return sub.id == id;
    });
  };
  while (exists(candidate)) {
    ++suffix;
    candidate = std::string(baseId) + "_copy" + std::to_string(suffix);
  }
  return candidate;
}

} // namespace app::ui
