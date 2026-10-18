#pragma once

namespace waybar::modules::hyprland {

constexpr int workspaceLayer(int workspace, int size) {
  return size > 0 && workspace > 0 && (workspace - 1) / size < 2 ? (workspace - 1) / size : -1;
}

constexpr int pairedWorkspace(int workspace, int size) {
  const int layer = workspaceLayer(workspace, size);
  return layer < 0 ? workspace : workspace + (layer == 0 ? size : -size);
}

}  // namespace waybar::modules::hyprland
