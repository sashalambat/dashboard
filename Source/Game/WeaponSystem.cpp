#include "Game/WeaponSystem.h"
#include "Game/World.h"

namespace sbs {

void resetWeapon(WeaponRuntime& w, WeaponId id, int upgradeLevel) {
    const WeaponDef& def = weaponDef(id);
    w.id = id;
    w.ammoInMag = def.magazine + upgradeLevel;
    w.reserve = def.magazine * (3 + upgradeLevel);
    w.cooldown = 0.0f;
    w.reloadLeft = 0.0f;
    w.heat = 0.0f;
    w.recoilPitch = 0.0f;
    w.reloading = false;
    if (def.energyHeat) {
        w.ammoInMag = def.magazine;
        w.reserve = 999;
    }
}

void tickWeapons(PlayerState& player, float dt) {
    for (WeaponRuntime& w : player.weapons) {
        w.cooldown = std::max(0.0f, w.cooldown - dt);
        w.recoilPitch = std::max(0.0f, w.recoilPitch - dt * 0.35f);
        if (w.reloading) {
            w.reloadLeft -= dt;
            if (w.reloadLeft <= 0.0f) {
                const WeaponDef& def = weaponDef(w.id);
                const int need = def.magazine - w.ammoInMag;
                const int take = std::min(need, w.reserve);
                w.ammoInMag += take;
                w.reserve -= take;
                w.reloading = false;
                w.reloadLeft = 0.0f;
            }
        }
        if (weaponDef(w.id).energyHeat) {
            w.heat = std::max(0.0f, w.heat - dt * 28.0f);
        }
    }
    player.aimSpread = std::max(0.0f, player.aimSpread - dt * 1.8f);
}

void fireWeapon(PlayerState& player, World& world, bool alt) {
    if (!player.alive || player.spectating) return;
    int slot = player.weaponSlot;
    if (alt) slot = 1;
    slot = clampi(slot, 0, 1);
    WeaponRuntime& w = player.weapons[slot];
    const WeaponDef& def = weaponDef(w.id);
    if (w.reloading || w.cooldown > 0.0f) return;
    if (def.energyHeat && w.heat >= 100.0f) return;
    if (!def.energyHeat && w.ammoInMag <= 0) {
        if (w.reserve > 0) {
            w.reloading = true;
            w.reloadLeft = def.reloadTime;
        }
        return;
    }

    player.firing = true;
    w.cooldown = def.fireInterval * (1.0f - player.upgradeLevel * 0.04f);
    w.recoilPitch += def.recoil;
    player.pitch = clampf(player.pitch + def.recoil, -1.2f, 1.2f);
    player.aimSpread = clampf(player.aimSpread + def.spread, 0.0f, 0.35f);
    if (def.energyHeat) {
        w.heat = std::min(100.0f, w.heat + 8.0f);
    } else {
        w.ammoInMag -= 1;
    }

    const Vec2 origin = player.pos + dirFromYaw(player.yaw) * 0.45f;
    if (def.hitscan) {
        const int pellets = std::max(1, def.pellets);
        for (int i = 0; i < pellets; ++i) {
            const float yaw = player.yaw + world.rng().range(-def.spread - player.aimSpread, def.spread + player.aimSpread);
            // Hitscan is resolved by World through a public helper via projectile-less path.
            world.addProjectile(player, w.id, origin, dirFromYaw(yaw)); // reused as hitscan marker if speed==0
        }
    } else {
        world.addProjectile(player, w.id, origin, dirFromYaw(player.yaw));
    }
}

} // namespace sbs
