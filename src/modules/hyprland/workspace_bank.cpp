#include "modules/hyprland/workspace_bank.hpp"

#include <array>
#include <charconv>
#include <string>

namespace waybar::modules::hyprland {

namespace {
std::optional<int> positiveNumber(std::string_view text) {
  if (text.empty() || text.front() < '1' || text.front() > '9') {
    return std::nullopt;
  }
  int number = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), number);
  return error == std::errc{} && end == text.data() + text.size() ? std::optional{number}
                                                                  : std::nullopt;
}
}

std::optional<int> workspaceBankSlot(const WorkspaceIdentity& identity) {
  auto number = identity.number();
  if (identity.kind == WorkspaceKind::Special) {
    std::string_view name = identity.name;
    if (name.starts_with("workspace-swap-")) {
      name.remove_prefix(std::string_view{"workspace-swap-"}.size());
      const auto separator = name.find('-');
      if (separator == std::string_view::npos) {
        return std::nullopt;
      }
      const auto storage = positiveNumber(name.substr(separator + 1));
      if (!storage || *storage < 3) {
        return std::nullopt;
      }
      name = name.substr(0, separator);
    }
    number = positiveNumber(name);
  }
  return number.has_value() && *number >= 1 && *number <= 10 ? number : std::nullopt;
}

std::optional<std::pair<int, int>> workspaceBankEvent(std::string_view payload) {
  const auto separator = payload.find(',');
  if (separator == std::string_view::npos) {
    return std::nullopt;
  }
  const auto slot = positiveNumber(payload.substr(0, separator));
  const auto bank = positiveNumber(payload.substr(separator + 1));
  if (!slot || *slot > 10 || !bank) {
    return std::nullopt;
  }
  return std::pair{*slot, *bank};
}

Json::Value workspaceBankWorkspaces(const Json::Value& workspaces) {
  Json::Value result(Json::arrayValue);
  std::array<bool, 10> present{};
  std::array<Json::Value, 10> banked;

  for (const auto& workspace : workspaces) {
    const auto identity = parseWorkspaceIdentity(workspace);
    const auto slot = identity.has_value() ? workspaceBankSlot(*identity) : std::nullopt;
    if (slot.has_value()) {
      if (identity->kind == WorkspaceKind::Special) {
        if (workspace.get("windows", 0).asInt() > 0 && banked[*slot - 1].isNull()) {
          banked[*slot - 1] = workspace;
        }
        continue;
      }
      present[*slot - 1] = true;
    }
    result.append(workspace);
  }

  for (int index = 0; index < 10; ++index) {
    if (present[index] || banked[index].get("windows", 0).asInt() == 0) {
      continue;
    }
    Json::Value workspace;
    workspace["address"] = std::to_string(index + 1);
    workspace["id"] = index + 1;
    workspace["type"] = "numbered";
    workspace["name"] = std::to_string(index + 1);
    workspace["monitor"] = banked[index]["monitor"];
    workspace["windows"] = 0;
    result.append(workspace);
  }
  return result;
}

}
