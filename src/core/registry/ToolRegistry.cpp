#include "core/registry/ToolRegistry.h"

#include <algorithm>
#include <utility>

namespace core::registry {

bool ToolRegistry::registerTool(ToolEntry entry) {
  if (entry.id.empty() || !entry.factory) {
    return false;
  }
  if (findById(entry.id) != nullptr || findByKind(entry.kind) != nullptr) {
    return false;
  }
  m_entries.push_back(std::move(entry));
  return true;
}

const ToolEntry* ToolRegistry::findById(std::string_view id) const noexcept {
  const auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const ToolEntry& entry) {
    return entry.id == id;
  });
  return it == m_entries.end() ? nullptr : &(*it);
}

const ToolEntry* ToolRegistry::findByKind(ToolKind kind) const noexcept {
  const auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const ToolEntry& entry) {
    return entry.kind == kind;
  });
  return it == m_entries.end() ? nullptr : &(*it);
}

std::unique_ptr<ITool> ToolRegistry::createById(std::string_view id) const {
  const ToolEntry* entry = findById(id);
  if (entry == nullptr) {
    return nullptr;
  }
  return entry->factory();
}

std::unique_ptr<ITool> ToolRegistry::createByKind(ToolKind kind) const {
  const ToolEntry* entry = findByKind(kind);
  if (entry == nullptr) {
    return nullptr;
  }
  return entry->factory();
}

} // namespace core::registry
