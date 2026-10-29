#include "TalentPacket.h"
#include <algorithm>
#include <limits>
#include <utility>

namespace TalentPackets {
bool Decode(const uint8_t* data, size_t size, uint32_t capacity, Snapshot& output, size_t& consumed) {
    if (!data || capacity < 1 || capacity > 8) return false;
    size_t position = 0;
    auto read = [&](uint32_t bytes, uint32_t& value) {
        if (bytes > size - position) return false;
        value = 0;
        for (uint32_t i = 0; i < bytes; ++i) value |= uint32_t(data[position++]) << (8 * i);
        return true;
    };
    Snapshot snapshot;
    uint32_t count, active, points;
    if (!read(4, points) || !read(1, count) || !read(1, active) || count > 8 ||
        (count ? active >= count : active != 0)) return false;
    snapshot.unspent = points > uint32_t(std::numeric_limits<int32_t>::max()) ? 0 : points;
    snapshot.groups.resize(std::min(count, capacity));
    snapshot.active = active < snapshot.groups.size() ? active : 0;
    for (uint32_t index = 0; index < count; ++index) {
        uint32_t talents;
        if (!read(1, talents)) return false;
        for (uint32_t i = 0; i < talents; ++i) {
            uint32_t id, rank;
            if (!read(4, id) || !read(1, rank) || !id || rank > 4) return false;
            if (index < capacity) snapshot.groups[index].talents.push_back({id, uint8_t(rank)});
        }
        uint32_t glyphs;
        if (!read(1, glyphs) || glyphs > 6) return false;
        for (uint32_t i = 0; i < glyphs; ++i) {
            uint32_t glyph;
            if (!read(2, glyph)) return false;
            if (index < capacity) snapshot.groups[index].glyphs[i] = uint16_t(glyph);
        }
    }
    output = std::move(snapshot);
    consumed = position;
    return true;
}
}
