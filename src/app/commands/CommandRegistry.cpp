#include "app/commands/CommandRegistry.h"

#include <algorithm>
#include <utility>

namespace app::commands {

bool CommandRegistry::registerCommand(CommandDescriptor descriptor, Handler handler) {
  if (descriptor.id.empty() || !handler) {
    return false;
  }

  const std::string id = descriptor.id;
  const bool isNew = m_entries.find(id) == m_entries.end();
  Entry entry {std::move(descriptor), std::move(handler)};
  m_entries[id] = std::move(entry);
  if (isNew) {
    m_order.push_back(id);
  }
  return true;
}

bool CommandRegistry::contains(std::string_view id) const noexcept {
  return m_entries.find(std::string(id)) != m_entries.end();
}

bool CommandRegistry::execute(std::string_view id) const {
  const auto it = m_entries.find(std::string(id));
  if (it == m_entries.end() || !it->second.descriptor.enabled || !it->second.handler) {
    return false;
  }
  return it->second.handler();
}

const CommandDescriptor* CommandRegistry::find(std::string_view id) const noexcept {
  const auto it = m_entries.find(std::string(id));
  if (it == m_entries.end()) {
    return nullptr;
  }
  return &it->second.descriptor;
}

std::vector<CommandDescriptor> CommandRegistry::commands() const {
  std::vector<CommandDescriptor> result;
  result.reserve(m_order.size());
  for (const std::string& id : m_order) {
    const auto it = m_entries.find(id);
    if (it != m_entries.end()) {
      result.push_back(it->second.descriptor);
    }
  }
  return result;
}

std::vector<CommandDescriptor> CommandRegistry::commandsInCategory(std::string_view category) const {
  std::vector<CommandDescriptor> result;
  for (const std::string& id : m_order) {
    const auto it = m_entries.find(id);
    if (it != m_entries.end() && it->second.descriptor.category == category) {
      result.push_back(it->second.descriptor);
    }
  }
  return result;
}

void CommandRegistry::clear() noexcept {
  m_entries.clear();
  m_order.clear();
}

} // namespace app::commands