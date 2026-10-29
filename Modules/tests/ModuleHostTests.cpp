#include "Modules/ModuleHost.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace ClientModules;
static std::vector<std::string> events;
#define CHECK(value) do { if (!(value)) { std::cerr << "Failed: " #value " at line " << __LINE__ << '\n'; std::exit(1); } } while (0)
static void UIReady() { events.push_back("ui.ready"); }
static void AStart() { events.push_back("a.start"); }
static void AStop() { events.push_back("a.stop"); }
static void BStart() { events.push_back("b.start"); }
static void BStop() { events.push_back("b.stop"); }
static void LuaUp(lua_State*) { events.push_back("lua+"); }
static void LuaDown(lua_State*) { events.push_back("lua-"); }
static void GameUp(uint32_t map) { CHECK(map == 571); events.push_back("game+"); }
static void GameDown() { events.push_back("game-"); }

int main() {
    std::string error;
    const Module a{"a", "1", {}, AStart, LuaUp, LuaDown, GameUp, GameDown, AStop, UIReady};
    const Module b{"b", "1", {"a"}, BStart, nullptr, nullptr, nullptr, nullptr, BStop};
    Host host;
    CHECK(host.Register(b, error));
    CHECK(host.Register(a, error));
    CHECK(!host.Register(a, error));
    CHECK(error.find("duplicate") != std::string::npos);
    CHECK(host.Start(error));
    CHECK(host.Modules().size() == 2 && host.Modules()[0] == &a);
    CHECK(host.Start(error)); // No repeated startup callbacks.
    CHECK(!host.Register(a, error));
    // Opaque stand-ins: the portable host never dereferences a Lua state.
    int first, second;
    auto* l1 = reinterpret_cast<lua_State*>(&first);
    auto* l2 = reinterpret_cast<lua_State*>(&second);
    host.LuaCreated(nullptr);
    host.LuaCreated(l1);
    host.UIReady(); host.UIReady();
    host.LuaCreated(l1);
    host.GameStarted(571);
    host.GameStarted(571);
    host.LuaDestroying(l2); // A foreign state must not detach the current state.
    host.LuaDestroying(l1);
    host.LuaCreated(l2); // /reloadui: rebind without restarting the game.
    host.UIReady();
    host.Stop(); // Also tears down an active game and Lua attachment.
    host.Stop();
    host.LuaDestroying(l2);
    host.GameStarted(571);
    CHECK((events == std::vector<std::string>{"a.start", "b.start", "lua+", "ui.ready", "game+", "lua-", "lua+", "ui.ready", "game-", "lua-", "b.stop", "a.stop"}));
    CHECK(host.Start(error)); // A stopped host can start again.
    host.LuaCreated(l1);
    host.LuaCreated(l2); // Replacement is paired even without an explicit detach.
    host.GameStarted(571);
    host.GameStopping();
    host.GameStopping();
    host.Stop();

    events.clear();
    Host missing;
    CHECK(missing.Register(b, error));
    CHECK(!missing.Start(error));
    CHECK(error.find("missing dependency") != std::string::npos);
    CHECK(events.empty() && missing.Modules().empty());
    CHECK(missing.Register(a, error));
    CHECK(missing.Start(error));
    missing.Stop();

    events.clear();
    const Module c{"c", "1", {"d"}, AStart};
    const Module d{"d", "1", {"c"}, BStart};
    Host cycle;
    CHECK(cycle.Register(c, error)); CHECK(cycle.Register(d, error));
    CHECK(!cycle.Start(error)); CHECK(error.find("cycle") != std::string::npos);
    CHECK(events.empty() && cycle.Modules().empty());
    const Module invalid{nullptr, "1", {}};
    CHECK(!cycle.Register(invalid, error));

    Host builtins;
    CHECK(RegisterBuiltInModules(builtins, error));
    CHECK(builtins.Start(error));
#ifdef EXPECT_EXAMPLE_MODULE
    CHECK(builtins.Modules().size() == 1);
    CHECK(std::string(builtins.Modules()[0]->id) == "example");
#else
    CHECK(builtins.Modules().empty());
#endif
    builtins.Stop();
    std::cout << "Client module lifecycle and registry tests passed\n";
}
