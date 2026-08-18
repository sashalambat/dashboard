#pragma once

#include "Engine/Protocol.h"
#include "Engine/SBSMath.h"
#include "Engine/SBSTypes.h"
#include "Game/GameConfig.h"

namespace sbs {

struct WeaponRuntime {
    WeaponId id = WeaponId::SBSAssaultRifle;
    int ammoInMag = 30;
    int reserve = 90;
    float cooldown = 0.0f;
    float reloadLeft = 0.0f;
    float heat = 0.0f;
    float recoilPitch = 0.0f;
    bool reloading = false;
};

struct PlayerState {
    uint8_t id = 0;
    char name[kMaxNameLen]{};
    bool active = false;
    bool isBot = false;
    bool alive = false;
    bool spectating = false;
    bool firing = false;
    Faction faction = Faction::SBSAlliance;
    PlayerClass cls = PlayerClass::Assault;
    TeamId team = TeamId::Alliance;
    BotDifficulty difficulty = BotDifficulty::Veteran;
    Vec2 pos;
    Vec2 vel;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float health = 100.0f;
    float armor = 50.0f;
    float respawn = 0.0f;
    int kills = 0;
    int deaths = 0;
    int assists = 0;
    int captures = 0;
    int score = 0;
    int pingMs = 12;
    int upgradeLevel = 0;
    WeaponRuntime weapons[2]{};
    int weaponSlot = 0;
    PlayerInput input{};
    float aimSpread = 0.0f;
    int pathIndex = 0;
    std::vector<Vec2> path;
    float botThink = 0.0f;
    uint8_t botTarget = 255;
    float nextFootstep = 0.0f;
};

struct ProjectileState {
    bool active = false;
    uint8_t id = 0;
    uint8_t owner = 0;
    WeaponId weapon = WeaponId::RocketLauncher;
    Vec2 pos;
    Vec2 vel;
    float life = 0.0f;
    float splash = 0.0f;
    float damage = 0.0f;
};

struct FlagState {
    TeamId owner = TeamId::Alliance;
    Vec2 home;
    Vec2 pos;
    int carrier = -1;
    bool atHome = true;
};

struct DomPoint {
    CellType type = CellType::Dom1;
    Vec2 pos;
    float alliance = 0.0f;
    float dominion = 0.0f;
    TeamId owner = TeamId::None;
};

struct CombatEvent {
    enum Kind { Shot, Hit, Kill, Explode, Pickup, Capture, Announcement } kind = Shot;
    uint8_t src = 0;
    uint8_t dst = 0;
    Vec2 pos;
    WeaponId weapon = WeaponId::SBSAssaultRifle;
    const char* text = "";
};

void resetWeapon(WeaponRuntime& w, WeaponId id, int upgradeLevel);
void fireWeapon(PlayerState& player, class World& world, bool alt);
void tickWeapons(PlayerState& player, float dt);

} // namespace sbs
