#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include <set>
#include <string>

#include "modules/hyprland/workspace_bank.hpp"

using namespace waybar::modules::hyprland;

namespace {
Json::Value workspace(const std::string& address, int windows = 0,
                      const std::string& monitor = "DP-1") {
  Json::Value value;
  value["address"] = address;
  value["name"] = address;
  value["type"] = address.starts_with("special:") ? "special" : "numbered";
  value["windows"] = windows;
  value["monitor"] = monitor;
  return value;
}

std::set<std::string> addresses(const Json::Value& workspaces) {
  std::set<std::string> result;
  for (const auto& item : workspaces) {
    const auto identity = parseWorkspaceIdentity(item);
    REQUIRE(identity.has_value());
    result.insert(identity->address);
  }
  return result;
}
}

TEST_CASE("Workspace bank slots match only plugin pairs", "[hyprland][workspace-bank]") {
  for (int slot = 1; slot <= 10; ++slot) {
    const auto number = std::to_string(slot);
    REQUIRE(workspaceBankSlot(*parseWorkspaceIdentity(workspace(number))) == slot);
    REQUIRE(workspaceBankSlot(*parseWorkspaceIdentity(workspace("special:" + number))) == slot);
    for (const int storage : {3, 4, 21}) {
      REQUIRE(workspaceBankSlot(*parseWorkspaceIdentity(workspace(
                  "special:workspace-swap-" + number + "-" + std::to_string(storage)))) == slot);
    }
  }
  for (const auto* address :
       {"0", "11", "14", "special:0", "special:11", "special:01", "special:terminal",
        "special:special:3", "special:workspace-swap-0-3", "special:workspace-swap-11-3",
        "special:workspace-swap-01-3", "special:workspace-swap-1-2", "special:workspace-swap-1-03",
        "special:workspace-swap-1-3-extra", "special:workspace-swap-1",
        "special:workspace-swap-1-999999999999"}) {
    REQUIRE_FALSE(workspaceBankSlot(*parseWorkspaceIdentity(workspace(address))).has_value());
  }
  REQUIRE_FALSE(workspaceBankSlot({"3", "3", WorkspaceKind::Named}).has_value());
}

TEST_CASE("Workspace bank events address independent slots and arbitrary banks",
          "[hyprland][workspace-bank]") {
  for (int slot = 1; slot <= 10; ++slot) {
    for (const int bank : {1, 2, 3, 25}) {
      const auto event = workspaceBankEvent(std::to_string(slot) + "," + std::to_string(bank));
      REQUIRE(event.has_value());
      REQUIRE(event->first == slot);
      REQUIRE(event->second == bank);
    }
  }
  for (const auto* payload : {"", "1", "2", "0,1", "11,1", "1,0", "1,-1", "01,2", "1,02", "1,2,3",
                              "1,", ",1", "1,3x", "1,999999999999"}) {
    REQUIRE_FALSE(workspaceBankEvent(payload).has_value());
  }
}

TEST_CASE("Workspace bank projection does not precreate slots", "[hyprland][workspace-bank]") {
  REQUIRE(workspaceBankWorkspaces(Json::Value(Json::arrayValue)).empty());
  Json::Value input(Json::arrayValue);
  input.append(workspace("1"));
  input.append(workspace("special:1"));
  input.append(workspace("special:7"));
  REQUIRE(addresses(workspaceBankWorkspaces(input)) == std::set<std::string>{"1"});
}

TEST_CASE("Hidden contents keep a normal button without exposing hidden windows",
          "[hyprland][workspace-bank]") {
  Json::Value input(Json::arrayValue);
  input.append(workspace("special:7", 2, "DP-2"));
  const auto projected = workspaceBankWorkspaces(input);
  REQUIRE(projected.size() == 1);
  REQUIRE(projected[0]["address"].asString() == "7");
  REQUIRE(projected[0]["monitor"].asString() == "DP-2");
  REQUIRE(projected[0]["windows"].asInt() == 0);
  REQUIRE_FALSE(projected[0].isMember("persistent-config"));
  REQUIRE(parseWorkspaceIdentity(projected[0])->kind == WorkspaceKind::Numbered);
  REQUIRE(parseWorkspaceIdentity(projected[0])->number() == 7);
  REQUIRE(input[0]["address"].asString() == "special:7");
}

TEST_CASE("Existing normal workspaces win over hidden counterparts", "[hyprland][workspace-bank]") {
  auto normal = workspace("7", 1, "DP-1");
  normal["name"] = "code";
  for (const bool specialFirst : {false, true}) {
    Json::Value input(Json::arrayValue);
    if (specialFirst) input.append(workspace("special:7", 4, "DP-2"));
    input.append(normal);
    if (!specialFirst) input.append(workspace("special:7", 4, "DP-2"));
    const auto projected = workspaceBankWorkspaces(input);
    REQUIRE(projected.size() == 1);
    REQUIRE(projected[0] == normal);
  }
}

TEST_CASE("Switching sparse banks preserves the button set", "[hyprland][workspace-bank]") {
  Json::Value first(Json::arrayValue);
  first.append(workspace("1", 1));
  first.append(workspace("7", 2, "DP-2"));
  first.append(workspace("special:3", 1));
  Json::Value second(Json::arrayValue);
  second.append(workspace("1"));
  second.append(workspace("3", 1));
  second.append(workspace("special:1", 1));
  second.append(workspace("special:7", 2, "DP-2"));
  const auto before = workspaceBankWorkspaces(first);
  const auto after = workspaceBankWorkspaces(second);
  REQUIRE(addresses(before) == std::set<std::string>{"1", "3", "7"});
  REQUIRE(addresses(before) == addresses(after));
  second[3]["windows"] = 0;
  REQUIRE(addresses(workspaceBankWorkspaces(second)) == std::set<std::string>{"1", "3"});
}

TEST_CASE("Multiple hidden banks share one button and empty storage cannot hide it",
          "[hyprland][workspace-bank]") {
  for (const bool emptyFirst : {false, true}) {
    Json::Value input(Json::arrayValue);
    const auto empty = workspace("special:7", 0, "DP-1");
    if (emptyFirst) input.append(empty);
    input.append(workspace("special:workspace-swap-7-3", 2, "DP-2"));
    input.append(workspace("special:workspace-swap-7-4", 1, "DP-2"));
    if (!emptyFirst) input.append(empty);
    const auto projected = workspaceBankWorkspaces(input);
    REQUIRE(addresses(projected) == std::set<std::string>{"7"});
    REQUIRE(projected[0]["windows"].asInt() == 0);
    REQUIRE(projected[0]["monitor"].asString() == "DP-2");
    input.append(workspace("7", 3, "DP-1"));
    const auto withNormal = workspaceBankWorkspaces(input);
    REQUIRE(withNormal.size() == 1);
    REQUIRE(withNormal[0]["windows"].asInt() == 3);
    REQUIRE(withNormal[0]["monitor"].asString() == "DP-1");
  }
}

TEST_CASE("Unrelated workspaces pass through bank projection", "[hyprland][workspace-bank]") {
  Json::Value input(Json::arrayValue);
  input.append(workspace("14", 1));
  input.append(workspace("special:terminal", 1));
  auto named = workspace("web", 1);
  named["type"] = "named";
  input.append(named);
  REQUIRE(workspaceBankWorkspaces(input) == input);
}

TEST_CASE("Legacy hidden workspace IDs project to numbered selectors",
          "[hyprland][workspace-bank]") {
  Json::Value input(Json::arrayValue);
  Json::Value special;
  special["id"] = -99;
  special["name"] = "special:10";
  special["monitor"] = "DP-2";
  special["windows"] = 1;
  input.append(special);
  const auto projected = workspaceBankWorkspaces(input);
  REQUIRE(addresses(projected) == std::set<std::string>{"10"});
  REQUIRE(parseWorkspaceIdentity(projected[0])->number() == 10);
}
