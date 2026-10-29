#pragma once
#include <cstdint>
struct lua_State;
namespace ClientModules { namespace XSpecs {
void Activate(uint32_t displayIndex);
int32_t ExtendTalentUI(lua_State* L);
} }
