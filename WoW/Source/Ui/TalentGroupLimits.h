#pragma once
#include <cstdint>

// The server's current talent membership mask has eight bits. Pet talent storage
// remains separate; these capacities apply to player and inspect groups only.
#ifdef WOW_MODULE_XSPECS
inline constexpr uint32_t TalentGroupCapacity = 8;
#else
inline constexpr uint32_t TalentGroupCapacity = 2;
#endif
inline constexpr uint32_t TalentWireGroupLimit = 8;
