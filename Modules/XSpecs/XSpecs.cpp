#include "Modules/ModuleHost.h"
#include "XSpecs.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/TalentGroupLimits.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <cstdio>
#include <cstring>

namespace ClientModules {
namespace XSpecs {
void Activate(uint32_t displayIndex) {
    const char* player = ClientServices::GetCharacterName();
    if (!player || !*player || displayIndex < 1 || displayIndex > TalentGroupCapacity) return;
    char payload[64];
    std::snprintf(payload, sizeof(payload), "SWPXSPEC\tSWITCH %u", displayIndex);
    CDataStore message;
    message.Put(static_cast<uint32_t>(CMSG_MESSAGECHAT));
    message.Put(static_cast<uint32_t>(CHAT_MSG_WHISPER));
    message.Put(uint32_t(0xFFFFFFFF));
    message.PutString(player);
    message.PutString(payload);
    message.Finalize();
    ClientServices::Send2(&message);
}
}
namespace {
int32_t Capacity(lua_State* L) { lua_pushinteger(L, TalentGroupCapacity); return 1; }

void RegisterLua(lua_State*) {
    FrameScript_RegisterFunction("XSpecs_GetCapacity", Capacity);
    FrameScript_RegisterFunction("XSpecs_ExtendTalentUI", XSpecs::ExtendTalentUI);
}
void UIReady() {
    static const char source[] = R"XSPECS(
local cap = XSpecs_GetCapacity()
local installed = false
local function Install()
    if installed or not PlayerTalentFrame then return end
    if not XSpecs_ExtendTalentUI() then return end
    for group=3,cap do
        local name = "PlayerSpecTab"..(group+1)
        if _G[name] then
            DEFAULT_CHAT_FRAME:AddMessage("X Specs: disable the old SWPMultispecs addon and reload the UI.")
            return
        end
        local tab = CreateFrame("CheckButton", name, PlayerTalentFrame, "PlayerSpecTabTemplate")
        tab:SetID(group+1)
        local specIndex = "spec"..group
        PlayerSpecTab_Load(tab, specIndex)
        -- The stock talent/glyph hotkeys select through this ordered list.
        if TALENT_SORT_ORDER then
            local found=false
            for _,key in ipairs(TALENT_SORT_ORDER) do if key==specIndex then found=true end end
            if not found then table.insert(TALENT_SORT_ORDER, math.min(group,#TALENT_SORT_ORDER+1),specIndex) end
        end
        local number = tab:CreateFontString(nil,"OVERLAY","GameFontHighlightSmall")
        number:SetPoint("BOTTOMRIGHT",tab,"BOTTOMRIGHT",0,0)
        number:SetText(group)
    end
    if TALENT_ACTIVATION_SPELLS then
        for group=1,cap do TALENT_ACTIVATION_SPELLS[group]=63645 end
    end
    -- Keep all player specs and the separate pet tab within the frame height.
    hooksecurefunc("PlayerTalentFrame_UpdateSpecs", function()
        local position=0
        for group=1,cap do
            local tab=_G["PlayerSpecTab"..(group<=2 and group or group+1)]
            if tab and tab:IsShown() then
                tab:SetSize(28,28);tab:ClearAllPoints()
                tab:SetPoint("TOPLEFT",PlayerTalentFrame,"TOPRIGHT",-32,-65-position*36)
                position=position+1
            end
        end
        if PlayerSpecTab3 and PlayerSpecTab3:IsShown() then
            PlayerSpecTab3:ClearAllPoints()
            PlayerSpecTab3:SetPoint("TOPLEFT",PlayerTalentFrame,"TOPRIGHT",-32,-81-position*36)
        end
    end)
    installed=true
end
local function Hello()
    local player=UnitName("player")
    if player then SendAddonMessage("SWPXSPEC","HELLO 1 "..cap,"WHISPER",player) end
end
local events=CreateFrame("Frame")
events:RegisterEvent("ADDON_LOADED")
events:RegisterEvent("PLAYER_ENTERING_WORLD")
events:SetScript("OnEvent",function(self,event,name)
    if event=="ADDON_LOADED" then
        if name=="Blizzard_TalentUI" then Install() end
    else Hello();Install() end
end)
Install()
if UnitName("player") then Hello() end
)XSPECS";
    FrameScript_Execute(source, "Native/XSpecs.lua", nullptr);
}
}
const Module& XSpecsModule() {
    static const Module module{"x-specs", "1.0.0", {}, nullptr, RegisterLua, nullptr,
        nullptr, nullptr, nullptr, UIReady};
    return module;
}
}
