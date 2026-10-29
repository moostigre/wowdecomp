#include "WoW/Source/Ui/TalentPacket.h"
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace TalentPackets;
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while (0)
void Put(std::vector<uint8_t>& bytes, uint32_t value, uint32_t width) {
    for (uint32_t i = 0; i < width; ++i) bytes.push_back(uint8_t(value >> (8 * i)));
}
std::vector<uint8_t> Packet(uint32_t count, uint32_t active) {
    std::vector<uint8_t> bytes;
    Put(bytes, 12, 4); Put(bytes, count, 1); Put(bytes, active, 1);
    for (uint32_t i = 0; i < count; ++i) {
        Put(bytes, 1, 1); Put(bytes, 1000 + i, 4); Put(bytes, i % 5, 1);
        Put(bytes, 6, 1);
        for (uint32_t j = 0; j < 6; ++j) Put(bytes, i * 10 + j, 2);
    }
    return bytes;
}
int main() {
    for (uint32_t count : {1, 2, 3, 5, 8}) {
        auto bytes = Packet(count, count - 1);
        Snapshot out; size_t used = 0;
        CHECK(Decode(bytes.data(), bytes.size(), 8, out, used));
        CHECK(out.groups.size() == count && out.active == count - 1 && used == bytes.size());
        CHECK(out.groups.back().talents[0].id == 1000 + count - 1);
        CHECK(out.groups.back().glyphs[5] == (count - 1) * 10 + 5);
        // The same packet must never expose a third index to a two-slot build.
        CHECK(Decode(bytes.data(), bytes.size(), 2, out, used));
        CHECK(out.groups.size() <= 2 && out.active < out.groups.size());
        for (size_t cut = 0; cut < bytes.size(); ++cut) {
            out.active = 123; used = 999;
            CHECK(!Decode(bytes.data(), cut, 8, out, used));
            CHECK(out.active == 123 && used == 999); // Failed parse is transactional.
        }
        const size_t end = bytes.size();
        bytes.push_back(99); bytes.push_back(88); // Inspect equipment follows talents.
        CHECK(Decode(bytes.data(), bytes.size(), 8, out, used) && used == end);
    }
    Snapshot out; size_t used = 0;
    auto bytes = Packet(5, 4);
    bytes[5] = 5; CHECK(!Decode(bytes.data(), bytes.size(), 8, out, used));
    bytes = Packet(5, 4); bytes[4] = 255; CHECK(!Decode(bytes.data(), bytes.size(), 8, out, used));
    bytes = Packet(1, 0); bytes[11] = 5; CHECK(!Decode(bytes.data(), bytes.size(), 8, out, used));
    bytes = Packet(1, 0); bytes[12] = 7; CHECK(!Decode(bytes.data(), bytes.size(), 8, out, used));
    bytes = Packet(0, 0); CHECK(Decode(bytes.data(), bytes.size(), 8, out, used) && out.groups.empty());
    CHECK(!Decode(bytes.data(), bytes.size(), 0, out, used));
    std::cout << "Passed native talent packets: 1/2/3/5/8 groups, glyphs, legacy bounds and malformed input\n";
}
