#pragma once

#include <json/value.h>

#include <optional>
#include <string_view>
#include <utility>

#include "modules/hyprland/workspace_identity.hpp"

namespace waybar::modules::hyprland {

std::optional<int> workspaceBankSlot(const WorkspaceIdentity& identity);
std::optional<std::pair<int, int>> workspaceBankEvent(std::string_view payload);
Json::Value workspaceBankWorkspaces(const Json::Value& workspaces);

}
