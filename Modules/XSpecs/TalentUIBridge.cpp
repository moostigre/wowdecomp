#include "XSpecs.h"
#include "WoW/Source/Ui/TalentGroupLimits.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>

namespace ClientModules { namespace XSpecs {
// Only the stock Talent UI's known tables are exposed to this installer. No
// general-purpose debug/upvalue mutation API is made available to addons.
bool PushTable(lua_State* L, const char* function, const char* table) {
    lua_getglobal(L, function);
    if (!lua_isfunction(L, -1) || lua_iscfunction(L, -1)) { lua_pop(L, 1); return false; }
    for (int i = 1; i <= 32; ++i) {
        const char* name = lua_getupvalue(L, -1, i);
        if (!name) break;
        if (!std::strcmp(name, table) && lua_istable(L, -1)) { lua_remove(L, -2); return true; }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return false;
}
int32_t ExtendTalentUI(lua_State* L) {
    const int base = lua_gettop(L);
    if (!PushTable(L, "PlayerSpecTab_OnClick", "specs") ||
        !PushTable(L, "PlayerSpecTab_Update", "talentSpecInfoCache")) {
        lua_settop(L, base); lua_pushboolean(L, 0); return 1;
    }
    const int specs = base + 1, cache = base + 2;
    lua_getfield(L, specs, "spec2");
    if (!lua_istable(L, -1)) { lua_settop(L, base); lua_pushboolean(L, 0); return 1; }
    const int prototype = base + 3;
    for (uint32_t group = 3; group <= TalentGroupCapacity; ++group) {
        char key[16], label[64];
        std::snprintf(key, sizeof(key), "spec%u", group);
        std::snprintf(label, sizeof(label), "Specialization %u", group);
        lua_newtable(L);
        const int descriptor = lua_gettop(L);
        lua_pushnil(L);
        while (lua_next(L, prototype)) {
            lua_pushvalue(L, -2); lua_pushvalue(L, -2); lua_settable(L, descriptor); lua_pop(L, 1);
        }
        lua_pushinteger(L, group); lua_setfield(L, descriptor, "talentGroup");
        for (const char* field : {"name", "tooltip", "glyphName"}) {
            lua_pushstring(L, label); lua_setfield(L, descriptor, field);
        }
        lua_setfield(L, specs, key);
        lua_newtable(L); lua_setfield(L, cache, key);
    }
    lua_settop(L, base); lua_pushboolean(L, 1); return 1;
}
}
}
