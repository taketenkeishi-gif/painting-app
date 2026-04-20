#pragma once

#include <string>
#include <string_view>
#include <vector>

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
  BlendMode,
  EraseMode,
  LockAlphaRespect
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

struct ToolBehaviorProfile {
  StrokeSettings stroke;
  BrushShapeSettings shape;
  StabilizerSettings stabilizer;
  VectorStrokeSettings vector;
  core::BlendMode blendMode {core::BlendMode::Normal};
  bool eraseMode {false};
  bool lockAlphaRespect {false};
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
  bool renameSubTool(core::ToolKind kind, std::string_view subToolId, const std::string& newDisplayName);
  bool removeSubTool(core::ToolKind kind, std::string_view subToolId);
  bool resetSubTool(core::ToolKind kind, std::string_view subToolId);

private:
  static std::string makeSubToolId(std::string_view baseId, const std::vector<SubToolDescriptor>& existing);

  std::vector<ToolDescriptor> m_defaultTools;
  std::vector<ToolDescriptor> m_tools;
};

} // namespace app::ui
