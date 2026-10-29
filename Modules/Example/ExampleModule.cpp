#include "Modules/ModuleHost.h"

namespace ClientModules {

const Module& ExampleModule() {
    // Disabled by default. Replace null callbacks with feature-specific behavior.
    static const Module module{"example", "1.0.0", {}};
    return module;
}

}
