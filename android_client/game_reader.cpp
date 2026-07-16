// CTF|认证
#include "game_reader.hpp"
#include "../shared/delta_protocol.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace off {
constexpr std::uint64_t GWorld = 0x1D0E3908;
constexpr std::uint64_t World_PersistentLevel = 0xF8;
constexpr std::uint64_t Level_ActorCluster = 0xC8;
constexpr std::uint64_t ActorCluster_Actors = 0x28;
constexpr std::uint64_t Actor_RootComponent = 0x180;
constexpr std::uint64_t Scene_RelativeLocation = 0x168;
constexpr std::uint64_t Pawn_PlayerState = 0x390;
constexpr std::uint64_t Character_Health = 0x2428;
constexpr std::uint64_t PlayerState_Dead = 0x4AC;
constexpr std::uint64_t PlayerState_Team = 0x658;
constexpr std::uint64_t PlayerState_Camp = 0x65C;
constexpr std::uint64_t PlayerState_Live = 0x93C;
constexpr std::uint64_t Health_HealthSet = 0x280;
constexpr std::uint64_t HealthSet_CurrentHealth = 0x3C;
constexpr std::uint64_t HealthSet_CurrentMaxHealth = 0x54;
}

struct TArray64 { std::uint64_t data; std::int32_t count; std::int32_t capacity; };
struct EncVector { float x, y, z; std::uint32_t handler; };
static bool user_pointer(std::uint64_t p) { return p >= 0x10000 && p < 0x800000000000ULL && !(p & 7); }

bool DeltaGameReader::pointer(std::int32_t pid, std::uint64_t at, std::uint64_t& value) {
    if (!driver_.read(pid, at, value) || !user_pointer(value)) { value = 0; return false; }
    return true;
}

bool DeltaGameReader::collect(std::int32_t pid, std::uint64_t libue4, FrameSnapshot& frame) {
    frame.pid = pid; frame.libue4 = libue4; frame.entities.clear(); ++frame.sequence;
    std::uint64_t world{}, level{}, cluster{};
    if (!pointer(pid, libue4 + off::GWorld, world) ||
        !pointer(pid, world + off::World_PersistentLevel, level) ||
        !pointer(pid, level + off::Level_ActorCluster, cluster)) {
        error_ = "world chain unavailable"; return false;
    }
    TArray64 actors{};
    if (!driver_.read(pid, cluster + off::ActorCluster_Actors, actors) ||
        !user_pointer(actors.data) || actors.count <= 0 || actors.count > 200000 || actors.capacity < actors.count) {
        error_ = "actor array invalid"; return false;
    }

    constexpr std::size_t batch = DELTA_MAX_READ_SIZE / sizeof(std::uint64_t);
    std::array<std::uint64_t, batch> pointers{};
    const int capped = std::min(actors.count, 20000);
    for (int start = 0; start < capped; start += static_cast<int>(batch)) {
        const int amount = std::min<int>(batch, capped - start);
        if (!driver_.read_process(pid, actors.data + start * sizeof(std::uint64_t), pointers.data(), amount * sizeof(std::uint64_t))) continue;
        for (int i = 0; i < amount; ++i) {
            const auto actor = pointers[i]; if (!user_pointer(actor)) continue;
            std::uint64_t player_state{}, health_component{}, health_set{}, root{};
            if (!pointer(pid, actor + off::Pawn_PlayerState, player_state)) continue;
            EntitySnapshot e{}; e.actor = actor;
            if (!driver_.read(pid, player_state + off::PlayerState_Team, e.team) ||
                !driver_.read(pid, player_state + off::PlayerState_Camp, e.camp) ||
                !driver_.read(pid, player_state + off::PlayerState_Live, e.live_status)) continue;
            std::uint8_t dead{}; driver_.read(pid, player_state + off::PlayerState_Dead, dead); e.dead = (dead & 1) != 0;
            if (e.team < -1 || e.team > 256 || e.camp < -1 || e.camp > 256) continue;

            if (pointer(pid, actor + off::Character_Health, health_component) &&
                pointer(pid, health_component + off::Health_HealthSet, health_set)) {
                driver_.read(pid, health_set + off::HealthSet_CurrentHealth, e.health);
                driver_.read(pid, health_set + off::HealthSet_CurrentMaxHealth, e.max_health);
                if (!std::isfinite(e.health) || !std::isfinite(e.max_health) || e.max_health < 0 || e.max_health > 100000)
                    e.health = e.max_health = 0;
            }
            if (pointer(pid, actor + off::Actor_RootComponent, root)) {
                EncVector v{};
                if (driver_.read(pid, root + off::Scene_RelativeLocation, v)) e.location = {v.x, v.y, v.z};
            }
            frame.entities.push_back(e);
        }
    }
    return true;
}
