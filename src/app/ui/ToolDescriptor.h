#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/selection/SelectionMask.h"
#include "core/tools/ToolTypes.h"
#include "core/tools/ToolType.h"

namespace app::ui {

enum class TargetLayerKind {
  Raster,
  Vector,
  Both
};

enum class CursorStyle {
  Default,
  Brush,
  Cross,
  Hand,
  Zoom,
  Fill
};

enum class ToolPropertyKey {
  Color,
  Size,
  Opacity,
  Hardness,
  Flow,
  Spacing,
  AntiAlias,
  Stabilization,
  PostCorrection,
  VelocityCorrection,
  ShapeType,
  Angle,
  Roundness,
  TaperStart,
  TaperEnd,
  StrokeWidth,
  SnapAngle,
  SimplifyLevel,
  VectorEraseMode,
  VectorTrimOutside,
  BlendMode,
  EraseMode,
  LockAlphaRespect,
  FillThreshold,
  FillContiguous,
  FillReferAllLayers,
  FillGapClose,
  SelectionMode,
  SelectionOp,
  AutoSelectThreshold,
  AutoSelectContiguous,
  AutoSelectReferAllLayers,
  SelectionFeather,
  SelectionAntiAlias,
  SelectionExpand,
  SelectionGapClose,
  SelectionEdgeSnap
};

struct StrokeSettings {
  int size {8};
  int opacity {100};
  int flow {100};
  int spacing {25};
  bool antiAlias {true};
};

struct BrushShapeSettings {
  core::BrushShapeType shapeType {core::BrushShapeType::Circle};
  int hardness {100};
  int angle {0};
  int roundness {100};
  int taperStart {0};
  int taperEnd {0};
};

struct StabilizerSettings {
  int stabilization {0};
  bool postCorrection {false};
  bool velocityBasedCorrection {false};
};

struct VectorStrokeSettings {
  int strokeWidth {8};
  int simplifyLevel {0};
  int snapAngle {0};
};

struct FillSettings {
  int threshold {0};
  bool contiguous {true};
  bool referAllLayers {false};
  int gapClose {0};
};

enum class SelectionMode {
  Rectangle,
  Lasso,
  PolygonLasso,  ///< クリック頂点追加、ダブルクリック確定
  AutoSelect,
  ObjectSelect,  ///< 全体類似色 / SAM2
};

enum class VectorEraserMode {
  TouchedOnly,
  ToIntersection,
  TrimOutside
};

struct SelectionSettings {
  SelectionMode mode {SelectionMode::Rectangle};
  core::SelectionOp op {core::SelectionOp::New};
  int autoSelectThreshold {16};
  bool autoSelectContiguous {true};
  bool autoSelectReferAllLayers {true};
  int featherRadius {0};
  bool antiAlias {true};
  int expandPixels {0};
  int gapCloseRadius {0};
  bool edgeSnap {false};
};

struct ToolBehaviorProfile {
  StrokeSettings stroke;
  BrushShapeSettings shape;
  StabilizerSettings stabilizer;
  VectorStrokeSettings vector;
  FillSettings fill;
  SelectionSettings selection;
  core::BlendMode blendMode {core::BlendMode::Normal};
  bool eraseMode {false};
  bool lockAlphaRespect {false};
  VectorEraserMode vectorEraseMode {VectorEraserMode::TouchedOnly};
  bool vectorTrimOutside {false};
  TargetLayerKind targetLayerKind {TargetLayerKind::Both};
  CursorStyle cursorStyle {CursorStyle::Default};
};

struct BrushPreset {
  int size {8};
  int opacity {100};
  int hardness {100};
  int flow {100};
  int spacing {25};
  bool antiAlias {true};
  int stabilization {0};
  bool postCorrection {false};
  bool velocityBasedCorrection {false};
  core::BrushShapeType shapeType {core::BrushShapeType::Circle};
  core::BlendMode blendMode {core::BlendMode::Normal};
  bool eraseMode {false};
  bool lockAlphaRespect {false};
  int angle {0};
  int roundness {100};
  int taperStart {0};
  int taperEnd {0};
  TargetLayerKind targetLayerKind {TargetLayerKind::Both};
  CursorStyle cursorStyle {CursorStyle::Default};
  int snapAngle {0};
  int simplifyLevel {0};
  int strokeWidth {0};
  int fillThreshold {0};
  bool fillContiguous {true};
  bool fillReferAllLayers {false};
  int fillGapClose {0};
  SelectionMode selectionMode {SelectionMode::Rectangle};
  core::SelectionOp selectionOp {core::SelectionOp::New};
  int autoSelectThreshold {16};
  bool autoSelectContiguous {true};
  bool autoSelectReferAllLayers {true};
  int selectionFeather {0};
  bool selectionAntiAlias {true};
  int selectionExpand {0};
  int selectionGapClose {0};
  bool selectionEdgeSnap {false};
  VectorEraserMode vectorEraseMode {VectorEraserMode::TouchedOnly};
  bool vectorTrimOutside {false};
  bool buildupMode {false};   // Krita非積み上げ / Photoshop積み上げ切替
  int gradientType {0};       // 0=Linear, 1=Radial
  int gradientFill {0};       // 0=ForegroundToBackground, 1=ForegroundToTransparent
};

struct SubToolDescriptor {
  std::string id;
  std::string displayName;
  BrushPreset preset;
  ToolBehaviorProfile profile;
  std::vector<ToolPropertyKey> editableProperties;
  std::string guide;
};

struct ToolDescriptor {
  core::ToolKind kind {core::ToolKind::Brush};
  std::string id;
  std::string displayName;
  std::vector<SubToolDescriptor> subTools;
  std::vector<ToolPropertyKey> availableProperties;
  std::string guide;
};

class ToolCatalog {
public:
  ToolCatalog();

  const std::vector<ToolDescriptor>& tools() const noexcept { return m_tools; }
  const ToolDescriptor* findTool(core::ToolKind kind) const noexcept;
  ToolDescriptor* findToolMutable(core::ToolKind kind) noexcept;
  const SubToolDescriptor* findSubTool(core::ToolKind kind, std::string_view subToolId) const noexcept;
  SubToolDescriptor* findSubToolMutable(core::ToolKind kind, std::string_view subToolId) noexcept;
  const SubToolDescriptor* defaultSubTool(core::ToolKind kind) const noexcept;
  bool duplicateSubTool(core::ToolKind kind, std::string_view sourceSubToolId, const std::string& newDisplayName);
  bool createSubTool(core::ToolKind kind, const std::string& newDisplayName);
  bool renameSubTool(core::ToolKind kind, std::string_view subToolId, const std::string& newDisplayName);
  bool removeSubTool(core::ToolKind kind, std::string_view subToolId);
  bool resetSubTool(core::ToolKind kind, std::string_view subToolId);

private:
  static std::string makeSubToolId(std::string_view baseId, const std::vector<SubToolDescriptor>& existing);

  std::vector<ToolDescriptor> m_defaultTools;
  std::vector<ToolDescriptor> m_tools;
};

} // namespace app::ui
