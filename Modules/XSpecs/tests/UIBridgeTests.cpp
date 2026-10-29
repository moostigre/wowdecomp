#include "Modules/XSpecs/XSpecs.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lualib.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << '\n'; std::exit(1); } } while (0)
int main(int argc, char** argv) {
    CHECK(argc == 2);
    lua_State* L = luaL_newstate(); luaL_openlibs(L);
    CHECK(luaL_dostring(L, "StaticPopupDialogs={};UIPanelWindows={};TALENT_ACTIVATION_SPELLS={};") == 0);
    if (luaL_dofile(L, argv[1])) { std::cerr << lua_tostring(L, -1) << '\n'; return 1; }
    CHECK(ClientModules::XSpecs::ExtendTalentUI(L) == 1);
    CHECK(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "PlayerSpecTab_OnClick");
    bool found = false;
    for (int i = 1; i < 32; ++i) {
        const char* name = lua_getupvalue(L, -1, i);
        if (!name) break;
        if (!std::strcmp(name, "specs")) {
            for (int group = 3; group <= 8; ++group) {
                char key[16]; std::snprintf(key, sizeof(key), "spec%d", group);
                lua_getfield(L, -1, key); CHECK(lua_istable(L, -1));
                lua_getfield(L, -1, "talentGroup"); CHECK(lua_tointeger(L, -1) == group); lua_pop(L, 1);
                lua_getfield(L, -1, "hasGlyphs"); CHECK(lua_toboolean(L, -1)); lua_pop(L, 2);
            }
            found = true;
        }
        lua_pop(L, 1);
    }
    CHECK(found);
    lua_close(L);
    std::cout << "Passed native UI table extension against stock 3.3.5 Talent UI\n";
}
