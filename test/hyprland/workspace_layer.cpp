#include "modules/hyprland/workspace_layer.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace waybar::modules::hyprland;

TEST_CASE("Workspace layers pair numeric workspaces", "[hyprland][workspace-layer]") {
  for (int workspace = 1; workspace <= 10; ++workspace) {
    REQUIRE(workspaceLayer(workspace, 10) == 0);
    REQUIRE(workspaceLayer(workspace + 10, 10) == 1);
    REQUIRE(pairedWorkspace(workspace, 10) == workspace + 10);
    REQUIRE(pairedWorkspace(workspace + 10, 10) == workspace);
  }
}

TEST_CASE("Workspace layers exclude other workspaces", "[hyprland][workspace-layer]") {
  for (const int workspace : {-99, 0, 21, 100}) {
    REQUIRE(workspaceLayer(workspace, 10) == -1);
    REQUIRE(pairedWorkspace(workspace, 10) == workspace);
  }
  REQUIRE(workspaceLayer(1, 0) == -1);
  REQUIRE(workspaceLayer(1, -10) == -1);
  REQUIRE(pairedWorkspace(1, 0) == 1);
}
