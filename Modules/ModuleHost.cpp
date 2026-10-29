#include "ModuleHost.h"
#include <cstring>

namespace ClientModules {

bool Host::Register(const Module& module, std::string& error) {
    error.clear();
    if (m_running || m_dispatching) { error = "cannot register modules after startup"; return false; }
    if (!module.id || !*module.id || !module.version || !*module.version) {
        error = "module ID and version are required"; return false;
    }
    for (const Module* existing : m_registered) {
        if (std::strcmp(existing->id, module.id) == 0) {
            error = std::string("duplicate module: ") + module.id; return false;
        }
    }
    m_registered.push_back(&module);
    return true;
}

bool Host::Visit(size_t index, std::vector<uint8_t>& marks, std::string& error) {
    if (marks[index] == 2) return true;
    const Module* module = m_registered[index];
    if (marks[index] == 1) { error = std::string("module dependency cycle at ") + module->id; return false; }
    marks[index] = 1;
    for (const char* dependency : module->dependencies) {
        size_t found = m_registered.size();
        for (size_t i = 0; dependency && i < m_registered.size(); ++i) {
            if (std::strcmp(dependency, m_registered[i]->id) == 0) { found = i; break; }
        }
        if (found == m_registered.size()) {
            error = std::string("missing dependency for ") + module->id + ": " + (dependency ? dependency : "<null>");
            return false;
        }
        if (!Visit(found, marks, error)) return false;
    }
    marks[index] = 2;
    m_order.push_back(module);
    return true;
}

bool Host::Start(std::string& error) {
    error.clear();
    if (m_dispatching) { error = "recursive module lifecycle transition"; return false; }
    if (m_running) return true;
    m_order.clear();
    std::vector<uint8_t> marks(m_registered.size(), 0);
    for (size_t i = 0; i < m_registered.size(); ++i) {
        if (!Visit(i, marks, error)) { m_order.clear(); return false; }
    }
    m_running = true;
    m_dispatching = true;
    for (const Module* module : m_order) if (module->start) module->start();
    m_dispatching = false;
    return true;
}

void Host::LuaCreated(lua_State* state) {
    if (!m_running || m_dispatching || !state || m_lua == state) return;
    // Pair destruction before replacing a state, including explicit UI reloads.
    if (m_lua) LuaDestroying(m_lua);
    m_lua = state;
    m_dispatching = true;
    for (const Module* module : m_order) if (module->luaCreated) module->luaCreated(state);
    m_dispatching = false;
}

void Host::LuaDestroying(lua_State* state) {
    if (m_dispatching || !state || state != m_lua) return;
    m_dispatching = true;
    for (auto it = m_order.rbegin(); it != m_order.rend(); ++it)
        if ((*it)->luaDestroying) (*it)->luaDestroying(state);
    m_lua = nullptr;
    m_dispatching = false;
}

void Host::GameStarted(uint32_t mapId) {
    if (!m_running || m_dispatching || m_game) return;
    m_game = true;
    m_dispatching = true;
    for (const Module* module : m_order) if (module->gameStarted) module->gameStarted(mapId);
    m_dispatching = false;
}

void Host::GameStopping() {
    if (m_dispatching || !m_game) return;
    m_dispatching = true;
    for (auto it = m_order.rbegin(); it != m_order.rend(); ++it)
        if ((*it)->gameStopping) (*it)->gameStopping();
    m_game = false;
    m_dispatching = false;
}

void Host::Stop() {
    if (!m_running || m_dispatching) return;
    GameStopping();
    LuaDestroying(m_lua);
    m_dispatching = true;
    for (auto it = m_order.rbegin(); it != m_order.rend(); ++it)
        if ((*it)->stop) (*it)->stop();
    m_running = false;
    m_dispatching = false;
}

Host& GetHost() {
    static Host host;
    return host;
}

}
