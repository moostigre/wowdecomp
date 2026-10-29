-- Harness for the actual stock Talent UI functions and the compiled-in script.
-- Frames are stand-ins; this does not test game rendering or secure execution.
local noop=function() end
local function texture()
    return setmetatable({}, {__index=function() return noop end})
end
local function frame(name,parent)
    local result={name=name,parent=parent,shown=true,normal=texture(),checkedTexture=texture()}
    function result:GetName() return self.name end
    function result:GetParent() return self.parent end
    function result:GetNormalTexture() return self.normal end
    function result:GetCheckedTexture() return self.checkedTexture end
    function result:SetChecked(value) self.checked=value end
    function result:Show() self.shown=true end
    function result:Hide() self.shown=false end
    function result:IsShown() return self.shown end
    function result:SetScript(event,fn) self[event]=fn end
    function result:SetPoint(...) self.point={...} end
    function result:CreateFontString() return texture() end
    setmetatable(result,{__index=function() return noop end})
    if name then _G[name]=result;_G[name..'Background']=texture() end
    return result
end
local events
function CreateFrame(kind,name,parent) local f=frame(name,parent);if not name then events=f end;return f end
local count,active=5,1
function GetNumTalentGroups(inspect,pet) return pet and 1 or count end
function GetActiveTalentGroup(inspect,pet) return pet and 1 or active end
function TalentFrame_UpdateSpecInfoCache(cache) cache.primaryTabIndex=0;cache.numTabs=3;cache.totalPointsSpent=0 end
SetPortraitTexture=noop
function hooksecurefunc(name,hook)
    local original=_G[name]
    _G[name]=function(...) local result=original(...);hook(...);return result end
end
TALENT_SORT_ORDER={'spec1','spec2','petspec1'}
TALENT_ACTIVATION_SPELLS={63644,63645}
DEFAULT_TALENT_SPEC='spec1'
DEFAULT_CHAT_FRAME={AddMessage=function(_,text) error(text) end}
local messages=0
function UnitName() return 'Tester' end
function SendAddonMessage(prefix,msg,channel,target)
    assert(prefix=='SWPXSPEC' and msg=='HELLO 1 8' and channel=='WHISPER' and target=='Tester')
    messages=messages+1
end
function PanelTemplates_GetSelectedTab() return 1 end
PlayerTalentFrame_Refresh=noop
PlayerTalentFrame=frame('PlayerTalentFrame')
PlayerTalentFrameActiveSpecTabHighlight=frame()
for index,key in ipairs({'spec1','spec2','petspec1'}) do
    PlayerSpecTab_Load(frame('PlayerSpecTab'..index,PlayerTalentFrame),key)
end
-- C++ executes the real embedded module script here, then invokes this check.
function CheckXSpecsUI()
    assert(messages==1)
    for i=1,8 do assert(TALENT_SORT_ORDER[i]=='spec'..i) end
    assert(TALENT_SORT_ORDER[9]=='petspec1')
    for _,n in ipairs({1,2,3,5,8}) do
        count=n
        for a=1,n do
            active=a
            PlayerTalentFrame_UpdateSpecs(active,count,1,1)
            local shown=0
            for i=1,8 do
                local tab=_G['PlayerSpecTab'..(i<=2 and i or i+1)]
                assert(tab:IsShown()==(i<=n))
                if tab:IsShown() then shown=shown+1 end
            end
            assert(shown==n and PlayerSpecTab3:IsShown())
            local tab=_G['PlayerSpecTab'..(a<=2 and a or a+1)]
            PlayerSpecTab_OnClick(tab)
            assert(PlayerTalentFrame.talentGroup==a and PlayerTalentFrame.pet==false)
        end
    end
    events.OnEvent(events,'ADDON_LOADED','Blizzard_TalentUI')
    events.OnEvent(events,'PLAYER_ENTERING_WORLD')
    assert(messages==2 and TALENT_SORT_ORDER[9]=='petspec1' and #TALENT_SORT_ORDER==9)
end
