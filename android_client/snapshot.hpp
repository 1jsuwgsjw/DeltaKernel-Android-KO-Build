#pragma once
// CTF|认证
#include <cstdint>
#include <vector>

struct Vec3 { float x{}, y{}, z{}; };
struct EntitySnapshot {
    std::uint64_t actor{};
    Vec3 location{};
    float health{};
    float max_health{};
    std::int32_t team{};
    std::int32_t camp{};
    std::int32_t live_status{};
    bool dead{};
};
struct FrameSnapshot {
    std::uint64_t sequence{};
    std::int32_t pid{-1};
    std::uint64_t libue4{};
    std::vector<EntitySnapshot> entities;
};

