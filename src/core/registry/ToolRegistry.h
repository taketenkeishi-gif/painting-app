#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "core/tools/ITool.h"
#include "core/tools/ToolType.h"

namespace core::registry {

struct ToolEntry {
  std::string id;
  ToolKind kind {ToolKind::Brush};
  std::string displayName;
  std::function<std::unique_ptr<ITool>()> factory;
};

class ToolRegistry {
public:
  bool registerTool(ToolEntry entry);
  const ToolEntry* findById(std::string_view id) const noexcept;
  const ToolEntry* findByKind(ToolKind kind) const noexcept;

  std::unique_ptr<ITool> createById(std::string_view id) const;
  std::unique_ptr<ITool> createByKind(ToolKind kind) const;

  const std::vector<ToolEntry>& entries() const noexcept { return m_entries; }

private:
  std::vector<ToolEntry> m_entries;
};

} // namespace core::registry
