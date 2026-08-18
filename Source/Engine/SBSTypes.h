#pragma once

#ifndef SBS_VERSION
#define SBS_VERSION "1.0.0"
#endif

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <memory>

namespace sbs {

constexpr const char* kGameTitle = "SBS Wars";
constexpr const char* kGameVersion = SBS_VERSION;
constexpr uint16_t kLanPort = 27015;
constexpr uint16_t kDiscoveryPort = 27016;
constexpr int kMaxPlayers = 64;
constexpr int kMinPlayers = 2;
constexpr int kTickRate = 60;
constexpr float kTickDt = 1.0f / static_cast<float>(kTickRate);
constexpr int kMaxProjectiles = 128;
constexpr int kMaxNameLen = 24;
constexpr uint32_t kProtocolMagic = 0x53425357; // 'SBSW'
constexpr uint16_t kProtocolVersion = 1;

enum class Faction : uint8_t {
    SBSAlliance = 0,
    CyberDominion = 1,
    Count
};

enum class PlayerClass : uint8_t {
    Assault = 0,
    Heavy = 1,
    Recon = 2,
    Engineer = 3,
    Medic = 4,
    Count
};

enum class WeaponId : uint8_t {
    SBSAssaultRifle = 0,
    SBSCombatShotgun = 1,
    SBSMachineGun = 2,
    SBSSniperRifle = 3,
    Railgun = 4,
    RocketLauncher = 5,
    PlasmaRifle = 6,
    PulseCannon = 7,
    Grenade = 8,
    Count
};

enum class GameModeId : uint8_t {
    Deathmatch = 0,
    TeamDeathmatch = 1,
    CaptureTheFlag = 2,
    Domination = 3,
    KingOfTheHill = 4,
    Count
};

enum class MatchState : uint8_t {
    Lobby = 0,
    Warmup = 1,
    Playing = 2,
    Ended = 3,
    Spectating = 4
};

enum class BotDifficulty : uint8_t {
    Recruit = 0,
    Veteran = 1,
    Elite = 2,
    Count
};

enum class TeamId : uint8_t {
    None = 0,
    Alliance = 1,
    Dominion = 2,
    Spectator = 3
};

enum class CellType : uint8_t {
    Empty = 0,
    Wall = 1,
    Cover = 2,
    SpawnA = 3,
    SpawnB = 4,
    FlagA = 5,
    FlagB = 6,
    Dom1 = 7,
    Dom2 = 8,
    Dom3 = 9,
    Hill = 10,
    Health = 11,
    Ammo = 12,
    Hazard = 13,
    Window = 14
};

enum class MapId : uint8_t {
    SBSFoundry = 0,
    ArcticBaseZeta = 1,
    OrbitalPlatformSeven = 2,
    CrimsonDesert = 3,
    CyberCore = 4,
    TitanFactory = 5,
    Count
};

inline const char* factionName(Faction f) {
    return f == Faction::CyberDominion ? "Cyber Dominion" : "SBS Alliance";
}

inline const char* className(PlayerClass c) {
    switch (c) {
        case PlayerClass::Assault: return "Assault";
        case PlayerClass::Heavy: return "Heavy";
        case PlayerClass::Recon: return "Recon";
        case PlayerClass::Engineer: return "Engineer";
        case PlayerClass::Medic: return "Medic";
        default: return "Unknown";
    }
}

inline const char* weaponName(WeaponId w) {
    switch (w) {
        case WeaponId::SBSAssaultRifle: return "SBS Assault Rifle";
        case WeaponId::SBSCombatShotgun: return "SBS Combat Shotgun";
        case WeaponId::SBSMachineGun: return "SBS Machine Gun";
        case WeaponId::SBSSniperRifle: return "SBS Sniper Rifle";
        case WeaponId::Railgun: return "Railgun";
        case WeaponId::RocketLauncher: return "Rocket Launcher";
        case WeaponId::PlasmaRifle: return "Plasma Rifle";
        case WeaponId::PulseCannon: return "Pulse Cannon";
        case WeaponId::Grenade: return "Grenade";
        default: return "Unknown";
    }
}

inline const char* modeName(GameModeId m) {
    switch (m) {
        case GameModeId::Deathmatch: return "Deathmatch";
        case GameModeId::TeamDeathmatch: return "Team Deathmatch";
        case GameModeId::CaptureTheFlag: return "Capture The Flag";
        case GameModeId::Domination: return "Domination";
        case GameModeId::KingOfTheHill: return "King Of The Hill";
        default: return "Unknown";
    }
}

inline const char* mapName(MapId m) {
    switch (m) {
        case MapId::SBSFoundry: return "SBS Foundry";
        case MapId::ArcticBaseZeta: return "Arctic Base Zeta";
        case MapId::OrbitalPlatformSeven: return "Orbital Platform Seven";
        case MapId::CrimsonDesert: return "Crimson Desert";
        case MapId::CyberCore: return "Cyber Core";
        case MapId::TitanFactory: return "Titan Factory";
        default: return "Unknown";
    }
}

inline const char* difficultyName(BotDifficulty d) {
    switch (d) {
        case BotDifficulty::Recruit: return "Recruit";
        case BotDifficulty::Veteran: return "Veteran";
        case BotDifficulty::Elite: return "Elite";
        default: return "Unknown";
    }
}

inline bool isTeamMode(GameModeId m) {
    return m != GameModeId::Deathmatch;
}

} // namespace sbs
