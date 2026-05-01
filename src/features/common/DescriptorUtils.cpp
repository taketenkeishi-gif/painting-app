#include "features/common/DescriptorUtils.h"

#include <utility>

namespace features::common {

app::ui::ToolBehaviorProfile makeProfile(const app::ui::BrushPreset& preset) {
  app::ui::ToolBehaviorProfile profile;
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
  profile.blendMode = preset.blendMode;
  profile.eraseMode = preset.eraseMode;
  profile.lockAlphaRespect = preset.lockAlphaRespect;
  profile.vectorEraseMode = preset.vectorEraseMode;
  profile.vectorTrimOutside = preset.vectorTrimOutside;
  profile.targetLayerKind = preset.targetLayerKind;
  profile.cursorStyle = preset.cursorStyle;
  return profile;
}

app::ui::SubToolDescriptor makeSubTool(
    std::string id,
    std::string displayName,
    app::ui::BrushPreset preset,
    std::vector<app::ui::ToolPropertyKey> editable,
    std::string guide) {
  app::ui::SubToolDescriptor descriptor;
  descriptor.id = std::move(id);
  descriptor.displayName = std::move(displayName);
  descriptor.preset = preset;
  descriptor.profile = makeProfile(preset);
  descriptor.editableProperties = std::move(editable);
  descriptor.guide = std::move(guide);
  return descriptor;
}

} // namespace features::common
