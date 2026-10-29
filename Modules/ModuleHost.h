#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct lua_State;

namespace ClientModules {

// Descriptors and their strings must outlive the host. All callbacks run on the
// client thread, must not throw, and must not initiate host lifecycle transitions.
struct Module {
    const char* id;
    const char* version;
    std::vector<const char*> dependencies;
    void (*start)() = nullptr;
    void (*luaCreated)(lua_State*) = nullptr;
    void (*luaDestroying)(lua_State*) = nullptr;
    void (*gameStarted)(uint32_t mapId) = nullptr;
    void (*gameStopping)() = nullptr;
    void (*stop)() = nullptr;
    void (*uiReady)() = nullptr;
};

class Host {
public:
    Host() = default;
    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;
    bool Register(const Module& module, std::string& error);
    bool Start(std::string& error);
    void LuaCreated(lua_State* state);
    void LuaDestroying(lua_State* state);
    void UIReady();
    void GameStarted(uint32_t mapId);
    void GameStopping();
    void Stop();
    const std::vector<const Module*>& Modules() const { return m_order; }

private:
    bool Visit(size_t index, std::vector<uint8_t>& marks, std::string& error);
    std::vector<const Module*> m_registered;
    std::vector<const Module*> m_order;
    lua_State* m_lua = nullptr;
    bool m_running = false;
    bool m_game = false;
    bool m_uiReady = false;
    bool m_dispatching = false;
};

// Explicit generated registry keeps enabled modules reachable by the linker.
bool RegisterBuiltInModules(Host& host, std::string& error);
Host& GetHost();

}
