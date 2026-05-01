#pragma once

#include <string>
#include <vector>

#include "app/ui/ToolDescriptor.h"

namespace features::common {

app::ui::ToolBehaviorProfile makeProfile(const app::ui::BrushPreset& preset);

app::ui::SubToolDescriptor makeSubTool(
    std::string id,
    std::string displayName,
    app::ui::BrushPreset preset,
    std::vector<app::ui::ToolPropertyKey> editable,
    std::string guide);

} // namespace features::common
