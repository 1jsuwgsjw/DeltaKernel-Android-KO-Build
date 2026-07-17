// CTF|认证
#include "game_reader.hpp"
#include "../shared/delta_protocol.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace off {
constexpr std::uint64_t GWorld = 0x1D0E3908;
constexpr std::uint64_t World_PersistentLevel = 0xF8;
constexpr std::uint64_t World_GameInstance = 0x190;
constexpr std::uint64_t GameInstance_LocalPlayers = 0x38;
constexpr std::uint64_t LocalPlayer_Controller = 0x30;
constexpr std::uint64_t Controller_CameraManager = 0x408;
constexpr std::uint64_t Camera_PrivateCache = 0x2BA0;
constexpr std::uint64_t CameraCache_POV = 0x10;
constexpr std::uint64_t POV_Location = 0x0;
constexpr std::uint64_t POV_Rotation = 0x10;
constexpr std::uint64_t POV_Fov = 0x1C;
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
constexpr std::uint64_t GUObjectArray = 0x1CDF2728;
constexpr std::uint64_t UObject_Class = 0x8;
constexpr std::uint64_t UObject_InternalIndex = 0x24;
constexpr std::uint64_t UStruct_Super = 0x40;
constexpr std::uint64_t ObjArray_ChunkTable = 0x20;
constexpr std::uint64_t ObjectItem_Size = 0x18;
constexpr std::uint64_t Pickup_Hidden = 0xF68;
constexpr std::uint64_t Pickup_IdName = 0xF98;
constexpr std::uint64_t Pickup_ItemID = 0x11F8;
constexpr std::uint64_t Pickup_ItemGid = 0x1210;
constexpr std::uint64_t Pickup_ItemCount = 0x1220;
constexpr std::uint64_t Pickup_EquipmentValue = 0x1708;
constexpr std::uint64_t Pickup_IsContainer = 0x1D30;
constexpr std::uint64_t Container_Type = 0x1E00;
constexpr int ClassIndex_InventoryPickup = 7495;
constexpr int ClassIndex_Container = 7496;
constexpr int ClassIndex_DeadBody = 7497;
constexpr int ClassIndex_OpenBox = 7500;
constexpr int ClassIndex_WeaponModule = 7502;
}

struct TArray64 { std::uint64_t data; std::int32_t count; std::int32_t capacity; };
struct EncVector { float x, y, z; std::uint32_t handler; };
static bool user_pointer(std::uint64_t p) { return p >= 0x10000 && p < 0x800000000000ULL && !(p & 7); }

static bool object_by_index(DriverClient& driver,std::int32_t pid,std::uint64_t base,int index,std::uint64_t& object){
    std::uint64_t chunks{},chunk{}; object=0;
    if(!driver.read(pid,base+off::GUObjectArray+off::ObjArray_ChunkTable,chunks)||!user_pointer(chunks))return false;
    if(!driver.read(pid,chunks+(index/65536)*8ULL,chunk)||!user_pointer(chunk))return false;
    if(!driver.read(pid,chunk+(index%65536)*off::ObjectItem_Size,object)||!user_pointer(object))return false;
    std::int32_t internal=-1;if(!driver.read(pid,object+off::UObject_InternalIndex,internal)||internal!=index){object=0;return false;}return true;
}

static bool class_is_a(DriverClient& driver,std::int32_t pid,std::uint64_t actor,std::uint64_t wanted){
    if(!wanted)return false;std::uint64_t cls{};if(!driver.read(pid,actor+off::UObject_Class,cls)||!user_pointer(cls))return false;
    for(int depth=0;depth<32&&user_pointer(cls);++depth){if(cls==wanted)return true;std::uint64_t super{};if(!driver.read(pid,cls+off::UStruct_Super,super)||super==cls)break;cls=super;}return false;
}

bool DeltaGameReader::pointer(std::int32_t pid, std::uint64_t at, std::uint64_t& value) {
    if (!driver_.read(pid, at, value) || !user_pointer(value)) { value = 0; return false; }
    return true;
}

bool DeltaGameReader::collect(std::int32_t pid, std::uint64_t libue4, FrameSnapshot& frame) {
    frame.pid = pid; frame.libue4 = libue4; frame.entities.clear(); frame.loot.clear(); ++frame.sequence;
    std::uint64_t world{}, level{}, cluster{};
    if (!driver_.read(pid, libue4 + off::GWorld, world)) {
        error_ = "GWorld read failed"; return false;
    }
    if (!world) {
        error_ = "GWorld is null (game world not initialized)"; return false;
    }
    if (!pointer(pid, world + off::World_PersistentLevel, level)) {
        error_ = "PersistentLevel unavailable"; return false;
    }
    if (!pointer(pid, level + off::Level_ActorCluster, cluster)) {
        error_ = "Level ActorCluster unavailable"; return false;
    }
    frame.camera = {};
    std::uint64_t game_instance{}, local_players_data{}, local_player{}, controller{}, camera_manager{};
    TArray64 local_players{};
    if (pointer(pid, world + off::World_GameInstance, game_instance) &&
        driver_.read(pid, game_instance + off::GameInstance_LocalPlayers, local_players) &&
        local_players.count > 0 && user_pointer(local_players.data) &&
        pointer(pid, local_players.data, local_player) &&
        pointer(pid, local_player + off::LocalPlayer_Controller, controller) &&
        pointer(pid, controller + off::Controller_CameraManager, camera_manager)) {
        EncVector camera_location{};
        const auto pov = camera_manager + off::Camera_PrivateCache + off::CameraCache_POV;
        driver_.read(pid, pov + off::POV_Location, camera_location);
        driver_.read(pid, pov + off::POV_Rotation, frame.camera.rotation);
        driver_.read(pid, pov + off::POV_Fov, frame.camera.fov);
        frame.camera.location = {camera_location.x, camera_location.y, camera_location.z};
        frame.camera.valid = std::isfinite(frame.camera.fov) && frame.camera.fov > 20.0f && frame.camera.fov < 180.0f;
    }
    TArray64 actors{};
    if (!driver_.read(pid, cluster + off::ActorCluster_Actors, actors) ||
        !user_pointer(actors.data) || actors.count <= 0 || actors.count > 200000 || actors.capacity < actors.count) {
        error_ = "actor array invalid"; return false;
    }

    std::uint64_t cls_pickup{},cls_container{},cls_deadbody{},cls_openbox{},cls_weapon_module{};
    object_by_index(driver_,pid,libue4,off::ClassIndex_InventoryPickup,cls_pickup);
    object_by_index(driver_,pid,libue4,off::ClassIndex_Container,cls_container);
    object_by_index(driver_,pid,libue4,off::ClassIndex_DeadBody,cls_deadbody);
    object_by_index(driver_,pid,libue4,off::ClassIndex_OpenBox,cls_openbox);
    object_by_index(driver_,pid,libue4,off::ClassIndex_WeaponModule,cls_weapon_module);

    constexpr std::size_t batch = DELTA_MAX_READ_SIZE / sizeof(std::uint64_t);
    std::array<std::uint64_t, batch> pointers{};
    const int capped = std::min(actors.count, 20000);
    for (int start = 0; start < capped; start += static_cast<int>(batch)) {
        const int amount = std::min<int>(batch, capped - start);
        if (!driver_.read_process(pid, actors.data + start * sizeof(std::uint64_t), pointers.data(), amount * sizeof(std::uint64_t))) continue;
        for (int i = 0; i < amount; ++i) {
            const auto actor = pointers[i]; if (!user_pointer(actor)) continue;
            const bool is_loot=class_is_a(driver_,pid,actor,cls_pickup);
            if(is_loot && frame.loot.size()<2048){
                LootSnapshot l{};l.actor=actor;
                std::uint64_t root{};if(pointer(pid,actor+off::Actor_RootComponent,root)){EncVector v{};if(driver_.read(pid,root+off::Scene_RelativeLocation,v))l.location={v.x,v.y,v.z};}
                std::uint8_t flags{};driver_.read(pid,actor+off::Pickup_Hidden,flags);l.hidden=(flags&1)!=0;
                driver_.read(pid,actor+off::Pickup_ItemID,l.category);driver_.read(pid,actor+off::Pickup_ItemID+4,l.sequence);
                driver_.read(pid,actor+off::Pickup_ItemGid,l.gid);driver_.read(pid,actor+off::Pickup_ItemCount,l.count);driver_.read(pid,actor+off::Pickup_EquipmentValue,l.value);
                l.container=class_is_a(driver_,pid,actor,cls_container);if(l.container){std::uint8_t t{};driver_.read(pid,actor+off::Container_Type,t);l.container_type=t;}
                l.important=l.value>=300000; l.name=std::string("物资 ")+std::to_string(l.category)+"-"+std::to_string(l.sequence);
                frame.loot.push_back(std::move(l));
                continue;
            }
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

