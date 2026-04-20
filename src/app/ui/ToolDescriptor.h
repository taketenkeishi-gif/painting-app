#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/tools/ToolTypes.h"
#include "core/tools/ToolType.h"

namespace app::ui {

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
  BlendMode,
  EraseMode,
  LockAlphaRespect
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
};

struct SubToolDescriptor {
  std::string id;
  std::string displayName;
  BrushPreset preset;
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
  const SubToolDescriptor* findSubTool(core::ToolKind kind, std::string_view subToolId) const noexcept;
  const SubToolDescriptor* defaultSubTool(core::ToolKind kind) const noexcept;

private:
  std::vector<ToolDescriptor> m_tools;
};

} // namespace app::ui
