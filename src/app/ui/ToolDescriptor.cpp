#include "app/ui/ToolDescriptor.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

#include "features/brush/Descriptor.h"
#include "features/eraser/Descriptor.h"
#include "features/eyedropper/Descriptor.h"
#include "features/fill/Descriptor.h"
#include "features/hand/Descriptor.h"
#include "features/line/Descriptor.h"
#include "features/move_layer/Descriptor.h"
#include "features/pen/Descriptor.h"
#include "features/rect_selection/Descriptor.h"
#include "features/zoom/Descriptor.h"
#include "features/gradient/Descriptor.h"
#include "features/comic/Descriptor.h"
#include "features/text/Descriptor.h"
#include "features/ruler/Descriptor.h"
#include "features/line_correction/Descriptor.h"
#include "features/operation/Descriptor.h"
#include "features/airbrush/Descriptor.h"
#include "features/color_mix/Descriptor.h"
#include "features/liquify/Descriptor.h"
#include "features/clone_stamp/Descriptor.h"
#include "features/sketch/Descriptor.h"

namespace app::ui {

namespace {

std::vector<ToolDescriptor> buildRawToolCatalog() {
  return {
      ::features::brush::makeBrushToolDescriptor(),
      ::features::eraser::makeEraserToolDescriptor(),
      ::features::eyedropper::makeEyedropperToolDescriptor(),
      ::features::fill::makeFillToolDescriptor(),
      ::features::pen::makePenToolDescriptor(),
      ::features::line::makeLineToolDescriptor(),
      ::features::rect_selection::makeRectSelectionToolDescriptor(),
      ::features::move_layer::makeMoveLayerToolDescriptor(),
      ::features::hand::makeHandToolDescriptor(),
      ::features::zoom::makeZoomToolDescriptor(),
      ::features::gradient::makeGradientToolDescriptor(),
      ::features::comic::makeComicToolDescriptor(),
      ::features::text::makeTextToolDescriptor(),
      ::features::ruler::makeRulerToolDescriptor(),
      ::features::line_correction::makeLineCorrectionToolDescriptor(),
      ::features::operation::makeOperationToolDescriptor(),
      ::features::airbrush::makeAirbrushToolDescriptor(),
      ::features::color_mix::makeColorMixToolDescriptor(),
      ::features::liquify::makeLiquifyToolDescriptor(),
      ::features::clone_stamp::makeCloneStampToolDescriptor(),
      ::features::sketch::makeSketchToolDescriptor()};
}

void appendUniqueSubTools(ToolDescriptor& target, const ToolDescriptor& source) {
  for (const SubToolDescriptor& sub : source.subTools) {
    const auto existing = std::find_if(target.subTools.begin(), target.subTools.end(), [&](const SubToolDescriptor& entry) {
      return entry.id == sub.id;
    });
    if (existing == target.subTools.end()) {
      target.subTools.push_back(sub);
    }
  }

  for (ToolPropertyKey key : source.availableProperties) {
    if (std::find(target.availableProperties.begin(), target.availableProperties.end(), key) ==
        target.availableProperties.end()) {
      target.availableProperties.push_back(key);
    }
  }
}

std::vector<ToolDescriptor> buildDefaultToolCatalog() {
  std::vector<ToolDescriptor> merged;
  for (const ToolDescriptor& descriptor : buildRawToolCatalog()) {
    auto existing = std::find_if(merged.begin(), merged.end(), [&](const ToolDescriptor& entry) {
      return entry.kind == descriptor.kind;
    });
    if (existing == merged.end()) {
      merged.push_back(descriptor);
      continue;
    }
    appendUniqueSubTools(*existing, descriptor);
  }
  return merged;
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

bool ToolCatalog::moveSubTool(core::ToolKind kind, std::size_t fromIndex, std::size_t toIndex) {
  ToolDescriptor* tool = findToolMutable(kind);
  if (tool == nullptr) {
    return false;
  }
  if (fromIndex >= tool->subTools.size() || toIndex >= tool->subTools.size() || fromIndex == toIndex) {
    return false;
  }
  SubToolDescriptor moved = std::move(tool->subTools[fromIndex]);
  tool->subTools.erase(tool->subTools.begin() + static_cast<std::ptrdiff_t>(fromIndex));
  tool->subTools.insert(tool->subTools.begin() + static_cast<std::ptrdiff_t>(toIndex), std::move(moved));
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
