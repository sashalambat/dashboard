#pragma once

#include "Engine/SBSTypes.h"
#include "Engine/SBSMath.h"

namespace sbs {

struct WeaponDef {
    WeaponId id = WeaponId::SBSAssaultRifle;
    const char* name = "";
    float damage = 30.0f;
    float fireInterval = 0.1f;
    int magazine = 30;
    float reloadTime = 1.8f;
    float spread = 0.04f;
    float recoil = 0.018f;
    float range = 40.0f;
    float projectileSpeed = 0.0f;
    float splashRadius = 0.0f;
    int pellets = 1;
    bool hitscan = true;
    bool piercing = false;
    bool energyHeat = false;
    bool isExplosive = false;
};

struct ClassDef {
    PlayerClass id = PlayerClass::Assault;
    const char* name = "";
    float maxHealth = 100.0f;
    float maxArmor = 50.0f;
    float speed = 4.6f;
    float radius = 0.28f;
    WeaponId primary = WeaponId::SBSAssaultRifle;
    WeaponId secondary = WeaponId::Grenade;
    float healPerSecond = 0.0f;
    float repairPerSecond = 0.0f;
    bool hasRadar = false;
    int upgradeSlots = 3;
};

struct DifficultyDef {
    BotDifficulty id = BotDifficulty::Veteran;
    float reaction = 0.28f;
    float accuracy = 0.55f;
    float aimSpeed = 4.5f;
    float aggression = 0.6f;
    float flankChance = 0.25f;
};

inline const WeaponDef& weaponDef(WeaponId id) {
    static const WeaponDef kDefs[] = {
        {WeaponId::SBSAssaultRifle, "SBS Assault Rifle", 28.0f, 0.095f, 30, 1.75f, 0.035f, 0.016f, 38.0f, 0, 0, 1, true, false, false, false},
        {WeaponId::SBSCombatShotgun, "SBS Combat Shotgun", 12.0f, 0.75f, 8, 2.4f, 0.16f, 0.06f, 12.0f, 0, 0, 8, true, false, false, false},
        {WeaponId::SBSMachineGun, "SBS Machine Gun", 22.0f, 0.072f, 100, 3.4f, 0.055f, 0.022f, 42.0f, 0, 0, 1, true, false, false, false},
        {WeaponId::SBSSniperRifle, "SBS Sniper Rifle", 92.0f, 1.15f, 5, 2.6f, 0.004f, 0.08f, 80.0f, 0, 0, 1, true, false, false, false},
        {WeaponId::Railgun, "Railgun", 110.0f, 1.45f, 4, 2.8f, 0.002f, 0.09f, 90.0f, 0, 0, 1, true, true, false, false},
        {WeaponId::RocketLauncher, "Rocket Launcher", 120.0f, 1.2f, 4, 2.9f, 0.02f, 0.1f, 55.0f, 18.0f, 2.4f, 1, false, false, false, true},
        {WeaponId::PlasmaRifle, "Plasma Rifle", 22.0f, 0.11f, 40, 0.0f, 0.03f, 0.014f, 32.0f, 22.0f, 0.4f, 1, false, false, true, false},
        {WeaponId::PulseCannon, "Pulse Cannon", 48.0f, 0.55f, 12, 2.1f, 0.025f, 0.05f, 28.0f, 16.0f, 1.4f, 1, false, false, false, true},
        {WeaponId::Grenade, "Grenade", 85.0f, 1.1f, 2, 1.4f, 0.05f, 0.04f, 18.0f, 10.0f, 2.6f, 1, false, false, false, true},
    };
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= static_cast<int>(WeaponId::Count)) {
        return kDefs[0];
    }
    return kDefs[idx];
}

inline const ClassDef& classDef(PlayerClass id) {
    static const ClassDef kDefs[] = {
        {PlayerClass::Assault, "Assault", 100.0f, 50.0f, 4.7f, 0.28f, WeaponId::SBSAssaultRifle, WeaponId::Grenade, 0, 0, false, 3},
        {PlayerClass::Heavy, "Heavy", 150.0f, 80.0f, 3.5f, 0.34f, WeaponId::SBSMachineGun, WeaponId::RocketLauncher, 0, 0, false, 3},
        {PlayerClass::Recon, "Recon", 80.0f, 25.0f, 5.6f, 0.24f, WeaponId::SBSSniperRifle, WeaponId::Railgun, 0, 0, true, 3},
        {PlayerClass::Engineer, "Engineer", 110.0f, 60.0f, 4.4f, 0.29f, WeaponId::SBSCombatShotgun, WeaponId::PulseCannon, 0, 18.0f, false, 3},
        {PlayerClass::Medic, "Medic", 90.0f, 40.0f, 4.9f, 0.27f, WeaponId::PlasmaRifle, WeaponId::Grenade, 14.0f, 0, false, 3},
    };
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= static_cast<int>(PlayerClass::Count)) {
        return kDefs[0];
    }
    return kDefs[idx];
}

inline const DifficultyDef& difficultyDef(BotDifficulty id) {
    static const DifficultyDef kDefs[] = {
        {BotDifficulty::Recruit, 0.55f, 0.28f, 2.4f, 0.35f, 0.05f},
        {BotDifficulty::Veteran, 0.28f, 0.62f, 5.0f, 0.65f, 0.28f},
        {BotDifficulty::Elite, 0.12f, 0.86f, 8.5f, 0.88f, 0.55f},
    };
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= static_cast<int>(BotDifficulty::Count)) {
        return kDefs[1];
    }
    return kDefs[idx];
}

struct ModeRules {
    GameModeId id = GameModeId::TeamDeathmatch;
    float matchSeconds = 600.0f;
    int scoreLimit = 50;
    int warmupSeconds = 8;
    bool teams = true;
    bool useFlags = false;
    bool useDomination = false;
    bool useHill = false;
};

inline ModeRules modeRules(GameModeId id) {
    switch (id) {
        case GameModeId::Deathmatch:
            return {id, 600.0f, 25, 8, false, false, false, false};
        case GameModeId::TeamDeathmatch:
            return {id, 600.0f, 50, 8, true, false, false, false};
        case GameModeId::CaptureTheFlag:
            return {id, 720.0f, 3, 8, true, true, false, false};
        case GameModeId::Domination:
            return {id, 720.0f, 200, 8, true, false, true, false};
        case GameModeId::KingOfTheHill:
            return {id, 600.0f, 100, 8, true, false, false, true};
        default:
            return {};
    }
}

inline WeaponId classWeapon(PlayerClass cls, int slot) {
    const ClassDef& def = classDef(cls);
    return slot == 0 ? def.primary : def.secondary;
}

} // namespace sbs
