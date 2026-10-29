# X Specs

Enable `WOW_MODULE_XSPECS=ON` to compile native player/inspection talent storage
for up to eight specializations. The server decides how many slots a character
has unlocked; the client does not grant talents, glyphs or unlocks. A server
configured for five sends five real talent groups. No Lua virtualization of the
third slot and no asset patch are needed.

The current server spell/talent membership masks are eight bits, so this version
supports 1–8 slots. Supporting more requires widening the server masks and
reviewing persistence, not just increasing the client array size.

## Server companion

Use `super-wow-project/mod-multispecs`, branch `feat/native-xspecs`, including its
updated core patch. A five-slot example is:

```
Multispecs.SpecCount = 5
Multispecs.UnlockLevels = "1,10,40,40,40"
```

The built-in client negotiates protocol version 1/capacity 8 using the existing
addon-message transport (`SWPXSPEC`, `HELLO 1 8`). The server then sends the normal
talent packet layout with the negotiated number of groups. Switching slots 1–8
uses `SWPXSPEC SWITCH N`; the server validates unlocks and player state and casts
the usual specialization activation spell. This also works for automatically unlocked characters that have not learned
Blizzard’s activation spells.

Unmodified clients receive at most two native groups. If their active character
slot is higher, the server projects that slot into the first visible group.
Inspection honors the viewing client's capacity, not the inspected player's.
Legacy dual/triple clients retain the existing compatibility messages. Use the
updated legacy addon or disable it with native builds; old third-slot wrappers
must not replace the native talent APIs.

## UI and lifecycle

The module is registered with CMake and installs its Lua bindings on every Lua
state creation. Its built-in UI script runs after FrameXML/addons are ready,
including `/reloadui`. It extends the stock 3.3.5 Talent UI's known specialization
and cache tables through a narrowly scoped native installer. Extra buttons use
the stock spec-tab template and preserve the separate pet tab, glyphs, talent
learning and preview APIs. Eight slots fit with compact tab spacing.

Skins that replace those stock closures/tables require compatibility work; the
installer refuses incompatible table shapes. The default stock Talent UI is the
reference. The native API/storage remains bounded even when a UI skin is absent.

When the module is disabled, player/inspection storage retains its original
capacity of two. The bounded packet parser still protects that build from excess
counts and invalid active indices. Malformed/truncated snapshots are rejected
before mutating the current cache, and inspection equipment bytes remain unread.

## Checks

```
cmake -S Modules/XSpecs/tests -B /tmp/xspecs-tests
cmake --build /tmp/xspecs-tests
ctest --test-dir /tmp/xspecs-tests --output-on-failure
```

With a system Lua 5.1 library, the test build also creates `XSpecsUIBridgeTests`.
Pass an extracted stock 3.3.5 `Blizzard_TalentUI.lua` to validate the native table
installer against real FrameXML functions; game assets are not redistributed.
To also execute the embedded module script with frame stand-ins and check tab
visibility/selection for 1, 2, 3, 5 and 8 specs (including the pet tab), pass:

```
/tmp/xspecs-tests/XSpecsUIBridgeTests /path/to/Blizzard_TalentUI.lua \
  Modules/XSpecs/XSpecs.cpp Modules/XSpecs/tests/UIBehavior.lua
```

The stand-ins do not validate rendered layouts or the game's secure UI rules.
The module-host tests verify UI-ready callbacks repeat after Lua recreation but
are delivered once per state. Windows builds and in-game checks are distinct:
verify learning, reset, activation, action bars, glyphs, inspection, relog and UI
reload on a matching test realm before deploying it to players.
