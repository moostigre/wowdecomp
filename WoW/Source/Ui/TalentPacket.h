#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace TalentPackets {
struct Talent { uint32_t id; uint8_t rank; };
struct Group { std::vector<Talent> talents; std::array<uint16_t, 6> glyphs{}; };
struct Snapshot {
    uint32_t unspent = 0;
    uint32_t active = 0;
    std::vector<Group> groups;
};
// Transactional and bounded. Trailing inspection equipment is not consumed.
bool Decode(const uint8_t* data, size_t size, uint32_t capacity, Snapshot& output, size_t& consumed);
}
