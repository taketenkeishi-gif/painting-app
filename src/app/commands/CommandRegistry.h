#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace app::commands {

struct CommandDescriptor {
  std::string id;
  std::string displayName;
  std::string category;
  std::string defaultShortcut;
  std::string description;
  bool enabled {true};
};

class CommandRegistry {
public:
  using Handler = std::function<bool()>;

  bool registerCommand(CommandDescriptor descriptor, Handler handler);
  bool contains(std::string_view id) const noexcept;
  bool execute(std::string_view id) const;
  const CommandDescriptor* find(std::string_view id) const noexcept;
  std::vector<CommandDescriptor> commands() const;
  std::vector<CommandDescriptor> commandsInCategory(std::string_view category) const;
  void clear() noexcept;

private:
  struct Entry {
    CommandDescriptor descriptor;
    Handler handler;
  };

  std::unordered_map<std::string, Entry> m_entries;
  std::vector<std::string> m_order;
};

} // namespace app::commands