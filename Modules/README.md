# Native client modules

Modules are C++17 sources compiled into the client, not injected DLLs. They do
not alter the 3.3.5a protocol, talent limits, stock archives, or signature policy.
No gameplay module is enabled by default. Optional gameplay modules such as
[X Specs](XSpecs/README.md) add their own client/server behavior.

## Add a module

Create `Modules/ThreeSpecs/CMakeLists.txt` and its sources. Add an option and
conditional `add_subdirectory(ThreeSpecs)` in `Modules/CMakeLists.txt` **before**
registry generation, following the disabled `Example` module:

```cmake
wow_add_client_module(NAME three-specs FACTORY ThreeSpecsModule
    SOURCES ThreeSpecsModule.cpp)
```

Provide `const ClientModules::Module& ClientModules::ThreeSpecsModule()` returning
a function-local static descriptor. `FACTORY` is an unqualified C++ identifier
in the `ClientModules` namespace. Module IDs must be unique. Dependencies name
other enabled module IDs; versions are metadata, not version-range constraints.
Sources are directly included in the `ClientModules` archive. Explicit generated
factory references prevent the linker dropping modules that only self-register.
All features are selected at configure/build time, not loaded from user files.

## Lifecycle contract

All host calls and callbacks execute synchronously on the existing client thread.
Callbacks must not throw or call host lifecycle/registration methods recursively;
recursive transitions are ignored (Start/Register return an error). Descriptors,
strings and callback code must outlive the host. Stop is explicit, not a static
object destructor, because engine services may already be gone at process exit.

The host checks the entire dependency graph before running any callbacks. It
starts dependencies first, preserving registration order where dependencies allow.
Game/Lua/stop teardown uses the reverse order. Duplicate IDs, missing dependencies
and cycles are rejected. Production startup logs a graph error and terminates;
it does not continue with an incomplete set of requested features.

- `start`: after client DB initialization, before the first Lua state. Network,
  Glue UI and world objects are not yet initialized. Reserve module-owned state.
- `luaCreated(L)`: after Lua libraries and compatibility helpers are ready, before
  Glue/FrameXML initialization. Register native Lua bindings here. Do not access
  frames or assume game-specific Lua APIs have already been registered.
- `luaDestroying(L)`: before Lua is closed (or early during client shutdown).
  Release registry references and Lua-owned state while L remains valid.
- `gameStarted(mapId)`: at the end of ClientInitializeGame after LoadMap and
  s_gameInitialized. This is game-system initialization, not a guarantee that a
  local player or every streamed world object is available. It is not a callback
  for every zone/map transfer; use existing game events for those.
- `gameStopping`: before ClientDestroyGame removes handlers/objects. Release
  references to game-session objects here.
- `stop`: after game teardown notifications and Lua detach, before
  WowClientDestroy tears down engine services. Do not access detached Lua state.

`FrameScript_Destroy/Initialize` also notify the host when `/reloadui` recreates
Lua. A UI reload does not restart modules or the game session. Lua binding hooks
must therefore work repeatedly and independently of gameStarted. Clients of the
FrameScript library that do not start the module host receive no callbacks.

## Gameplay modules

[X Specs](XSpecs/README.md) extends native talent storage, APIs and stock UI for
server-driven 1–8 player specializations. Its server companion handles unlocks,
activation, persistence and negotiated packet counts. Pet talent groups remain
separate. Networking extensions must preserve core handlers: NetClient has one
handler slot per opcode, so registering an existing opcode replaces its owner.

## Validation

Portable lifecycle and generated registry tests (no game assets required):

```sh
cmake -S Modules -B /tmp/wow-module-tests -DWOW_MODULE_TESTS=ON
cmake --build /tmp/wow-module-tests
ctest --test-dir /tmp/wow-module-tests --output-on-failure
```

Repeat with `-DWOW_MODULE_EXAMPLE=ON` in a different build directory to verify
that a selected module is actually pulled into the executable by the registry.
These tests are independent of Windows and do not validate the full client.
For a client build, use the repository's normal Windows/MSVC instructions; the
same `WOW_MODULE_*` options apply. Verify login, game entry/exit, reconnect,
`/reloadui`, and shutdown on Windows before shipping a feature module.

`uiReady` is delivered once per Lua state after game FrameXML and addons have
loaded, before game UI EnterWorld. Use it to create UI frames; Lua creation hooks
remain the correct place for native API registration. See [X Specs](XSpecs/README.md)
for the first gameplay extension, including the paired server requirements.
