#include "Game/World.h"

#include "Game/GameModes.h"
#include "Engine/SBSLog.h"

#include <cstring>

namespace sbs {
namespace {

int16_t quantize(float v) {
    return static_cast<int16_t>(clampf(v * 32.0f, -32767.0f, 32767.0f));
}

uint8_t yawByte(float yaw) {
    float n = (wrapAngle(yaw) + 3.14159265f) / 6.2831853f;
    n = clampf(n, 0.0f, 1.0f);
    return static_cast<uint8_t>(n * 255.0f);
}

bool sameTeam(const PlayerState& a, const PlayerState& b, GameModeId mode) {
    if (!isTeamMode(mode)) return false;
    return a.team == b.team && a.team != TeamId::None && a.team != TeamId::Spectator;
}

} // namespace

void World::reset(const MatchConfig& cfg, const GameMap& map) {
    cfg_ = cfg;
    ownedMap_ = map;
    map_ = &ownedMap_;
    rules_ = modeRules(cfg_.mode);
    if (cfg_.timeLimit > 0.0f) rules_.matchSeconds = cfg_.timeLimit;
    if (cfg_.scoreLimit > 0) rules_.scoreLimit = cfg_.scoreLimit;
    if (cfg_.warmupSeconds >= 0) rules_.warmupSeconds = cfg_.warmupSeconds;
    if (rules_.warmupSeconds <= 0) {
        state_ = MatchState::Playing;
        warmupLeft_ = 0.0f;
        lastAnnounce_ = "Match start";
    } else {
        state_ = MatchState::Warmup;
        warmupLeft_ = static_cast<float>(rules_.warmupSeconds);
        lastAnnounce_ = "Warmup";
    }
    tick_ = 0;
    timeLeft_ = rules_.matchSeconds;
    scoreA_ = 0;
    scoreB_ = 0;
    events_.clear();
    players_ = {};
    projectiles_ = {};
    hill_ = map_->hillPosition();
    hillOwner_ = TeamId::None;
    hillHold_ = 0.0f;
    flagA_ = {TeamId::Alliance, map_->flagPosition(TeamId::Alliance), map_->flagPosition(TeamId::Alliance), -1, true};
    flagB_ = {TeamId::Dominion, map_->flagPosition(TeamId::Dominion), map_->flagPosition(TeamId::Dominion), -1, true};
    int di = 0;
    for (const auto& o : map_->objectives()) {
        if (o.type == CellType::Dom1 || o.type == CellType::Dom2 || o.type == CellType::Dom3) {
            if (di < 3) {
                dom_[static_cast<size_t>(di)] = {o.type, o.pos, 0, 0, TeamId::None};
                ++di;
            }
        }
    }
    announce(std::string("Match setup: ") + modeName(cfg_.mode) + " on " + map_->name());
    placePickups();
}

int World::addPlayer(const std::string& name, PlayerClass cls, Faction faction, bool bot, BotDifficulty diff) {
    for (int i = 0; i < kMaxPlayers; ++i) {
        if (players_[static_cast<size_t>(i)].active) continue;
        PlayerState& p = players_[static_cast<size_t>(i)];
        p = PlayerState{};
        p.id = static_cast<uint8_t>(i);
        p.active = true;
        p.isBot = bot;
        p.cls = cls;
        p.faction = faction;
        p.difficulty = diff;
        p.team = isTeamMode(cfg_.mode)
                     ? ((faction == Faction::CyberDominion) ? TeamId::Dominion : TeamId::Alliance)
                     : TeamId::None;
        std::memset(p.name, 0, sizeof(p.name));
        for (size_t i = 0; i + 1 < kMaxNameLen && i < name.size(); ++i) {
            p.name[i] = name[i];
        }
        resetWeapon(p.weapons[0], classWeapon(cls, 0), p.upgradeLevel);
        resetWeapon(p.weapons[1], classWeapon(cls, 1), p.upgradeLevel);
        spawnPlayer(p);
        return i;
    }
    return -1;
}

void World::placePickups() {
    pickups_ = {};
    if (!map_) return;
    static const WeaponId kCycle[] = {
        WeaponId::SBSAssaultRifle, WeaponId::SBSCombatShotgun, WeaponId::SBSMachineGun,
        WeaponId::SBSSniperRifle, WeaponId::Railgun, WeaponId::RocketLauncher,
        WeaponId::PlasmaRifle, WeaponId::PulseCannon, WeaponId::Grenade
    };
    int n = 0;
    for (int y = 2; y < map_->height() - 2 && n < static_cast<int>(pickups_.size()); ++y) {
        for (int x = 2; x < map_->width() - 2 && n < static_cast<int>(pickups_.size()); ++x) {
            if (map_->isBlocked(x, y)) continue;
            const CellType t = map_->cell(x, y);
            const bool ammoPad = t == CellType::Ammo || t == CellType::Health;
            if (!ammoPad && ((x * 17 + y * 11) % 13) != 0) continue;
            WeaponPickup& u = pickups_[static_cast<size_t>(n)];
            u.active = true;
            u.id = kCycle[n % 9];
            u.pos = {static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
            u.respawn = 0.0f;
            ++n;
        }
    }
}

void World::tickPickups(float dt) {
    if (state_ != MatchState::Playing && state_ != MatchState::Warmup) return;
    for (auto& u : pickups_) {
        if (!u.active) {
            u.respawn -= dt;
            if (u.respawn <= 0.0f) u.active = true;
            continue;
        }
        for (auto& p : players_) {
            if (!p.active || !p.alive || p.spectating) continue;
            if (distanceSq(p.pos, u.pos) > 0.55f * 0.55f) continue;
            if (p.weapons[0].id == u.id) {
                p.weapons[0].reserve = std::min(p.weapons[0].reserve + weaponDef(u.id).magazine * 2, 400);
            } else if (p.weapons[1].id == u.id) {
                p.weapons[1].reserve = std::min(p.weapons[1].reserve + weaponDef(u.id).magazine * 2, 400);
                p.weaponSlot = 1;
            } else {
                resetWeapon(p.weapons[1], u.id, p.upgradeLevel);
                p.weaponSlot = 1;
            }
            events_.push_back({CombatEvent::Pickup, p.id, 0, u.pos, u.id, "pickup"});
            announce(std::string(p.name) + " picked up " + weaponName(u.id));
            u.active = false;
            u.respawn = 22.0f;
            break;
        }
    }
}

void World::removePlayer(uint8_t id) {
    if (id >= kMaxPlayers) return;
    players_[id].active = false;
    players_[id].alive = false;
}

PlayerState* World::playerById(uint8_t id) {
    if (id >= kMaxPlayers || !players_[id].active) return nullptr;
    return &players_[id];
}

const PlayerState* World::playerById(uint8_t id) const {
    if (id >= kMaxPlayers || !players_[id].active) return nullptr;
    return &players_[id];
}

void World::spawnPlayer(PlayerState& p) {
    if (p.spectating) {
        p.alive = false;
        return;
    }
    const ClassDef& def = classDef(p.cls);
    p.alive = true;
    p.health = def.maxHealth;
    p.armor = def.maxArmor;
    p.respawn = 0.0f;
    p.firing = false;
    p.pos = map_->randomSpawn(isTeamMode(cfg_.mode) ? p.team : TeamId::None, rng_);
    p.yaw = rng_.range(-3.14f, 3.14f);
    p.vel = {};
    resetWeapon(p.weapons[0], classWeapon(p.cls, 0), p.upgradeLevel);
    resetWeapon(p.weapons[1], classWeapon(p.cls, 1), p.upgradeLevel);
}

void World::announce(const std::string& text) {
    lastAnnounce_ = text;
    events_.push_back({CombatEvent::Announcement, 0, 0, {}, WeaponId::SBSAssaultRifle, lastAnnounce_.c_str()});
    logInfo(text);
}

void World::addScore(PlayerState& p, int amount) {
    p.score += amount;
    if (isTeamMode(cfg_.mode)) {
        if (p.team == TeamId::Alliance) scoreA_ += amount;
        if (p.team == TeamId::Dominion) scoreB_ += amount;
    }
}

void World::killPlayer(PlayerState& victim, PlayerState* killer, WeaponId weapon) {
    if (!victim.alive) return;
    victim.alive = false;
    victim.health = 0;
    victim.deaths += 1;
    victim.respawn = 3.5f;
    victim.firing = false;
    if (flagA_.carrier == victim.id) {
        flagA_.carrier = -1;
        flagA_.atHome = false;
        flagA_.pos = victim.pos;
    }
    if (flagB_.carrier == victim.id) {
        flagB_.carrier = -1;
        flagB_.atHome = false;
        flagB_.pos = victim.pos;
    }
    if (killer && killer != &victim) {
        killer->kills += 1;
        addScore(*killer, 1);
        events_.push_back({CombatEvent::Kill, killer->id, victim.id, victim.pos, weapon, "kill"});
    } else {
        events_.push_back({CombatEvent::Kill, victim.id, victim.id, victim.pos, weapon, "suicide"});
    }
}

void World::applyDamage(PlayerState& victim, float amount, PlayerState* src, WeaponId weapon) {
    if (!victim.alive || amount <= 0.0f) return;
    if (src && sameTeam(*src, victim, cfg_.mode) && src != &victim) return;
    float remaining = amount;
    const float armorAbsorb = std::min(victim.armor, remaining * 0.45f);
    victim.armor -= armorAbsorb;
    remaining -= armorAbsorb;
    victim.health -= remaining;
    events_.push_back({CombatEvent::Hit, src ? src->id : static_cast<uint8_t>(0), victim.id, victim.pos, weapon, "hit"});
    if (src && src->cls == PlayerClass::Medic && src->team == victim.team && isTeamMode(cfg_.mode) && src != &victim) {
        // medics deal no friendly damage; heal instead if they somehow hit allies
        victim.health = std::min(classDef(victim.cls).maxHealth, victim.health + amount * 0.25f);
        return;
    }
    if (victim.health <= 0.0f) {
        killPlayer(victim, src, weapon);
    }
}

void World::addProjectile(const PlayerState& owner, WeaponId weapon, const Vec2& origin, const Vec2& dir) {
    const WeaponDef& def = weaponDef(weapon);
    if (def.hitscan) {
        const Vec2 n = dir.normalized();
        Vec2 p = origin;
        PlayerState* hit = nullptr;
        for (int i = 0; i < 80; ++i) {
            p += n * 0.45f;
            if (map_->blocksSight(static_cast<int>(p.x), static_cast<int>(p.y))) break;
            for (auto& other : players_) {
                if (!other.active || !other.alive || other.id == owner.id) continue;
                if (distanceSq(p, other.pos) < 0.42f * 0.42f) {
                    hit = &other;
                    break;
                }
            }
            if (hit) break;
            if (distance(origin, p) > def.range) break;
        }
        events_.push_back({CombatEvent::Shot, owner.id, hit ? hit->id : static_cast<uint8_t>(255), p, weapon, "shot"});
        if (hit) {
            const float falloff = clampf(1.0f - distance(origin, hit->pos) / def.range, 0.2f, 1.0f);
            applyDamage(*hit, def.damage * falloff, const_cast<PlayerState*>(&owner), weapon);
        }
        return;
    }

    for (auto& pr : projectiles_) {
        if (pr.active) continue;
        pr.active = true;
        pr.id = static_cast<uint8_t>(&pr - &projectiles_[0]);
        pr.owner = owner.id;
        pr.weapon = weapon;
        pr.pos = origin;
        pr.vel = dir.normalized() * def.projectileSpeed;
        pr.life = 2.8f;
        pr.splash = def.splashRadius;
        pr.damage = def.damage;
        return;
    }
}

PlayerState* World::closestEnemy(const PlayerState& from, float maxDist, bool requireLos) {
    PlayerState* best = nullptr;
    float bestD = maxDist * maxDist;
    for (auto& p : players_) {
        if (!p.active || !p.alive || p.spectating || p.id == from.id) continue;
        if (sameTeam(from, p, cfg_.mode)) continue;
        const float d = distanceSq(from.pos, p.pos);
        if (d > bestD) continue;
        if (requireLos && !map_->lineOfSight(from.pos, p.pos)) continue;
        bestD = d;
        best = &p;
    }
    return best;
}

void World::setInput(uint8_t id, const PlayerInput& input) {
    PlayerState* p = playerById(id);
    if (!p) return;
    p->input = input;
    if (input.requestSpectate) {
        p->spectating = true;
        p->alive = false;
        p->team = TeamId::Spectator;
    }
}

void World::hitscan(PlayerState& attacker, const WeaponDef& def, const Vec2& origin, float yaw) {
    addProjectile(attacker, def.id, origin, dirFromYaw(yaw));
}

int World::humanCount() const {
    int n = 0;
    for (const auto& p : players_) if (p.active && !p.isBot) ++n;
    return n;
}

int World::botCount() const {
    int n = 0;
    for (const auto& p : players_) if (p.active && p.isBot) ++n;
    return n;
}

int World::aliveCount() const {
    int n = 0;
    for (const auto& p : players_) if (p.active && p.alive) ++n;
    return n;
}

void World::tickPlayers(float dt) {
    for (auto& p : players_) {
        if (!p.active) continue;
        p.firing = false;
        if (p.isBot) tickBot(*this, p, dt);
        if (p.spectating) continue;
        if (!p.alive) {
            p.respawn -= dt;
            if (p.respawn <= 0.0f && (state_ == MatchState::Playing || state_ == MatchState::Warmup)) spawnPlayer(p);
            continue;
        }

        p.weaponSlot = clampi(static_cast<int>(p.input.weaponSlot), 0, 1);
        if (p.input.classSelect > 0 && p.input.classSelect <= static_cast<uint8_t>(PlayerClass::Count)) {
            p.cls = static_cast<PlayerClass>(p.input.classSelect - 1);
        }
        p.yaw = p.input.yaw != 0.0f || p.isBot ? (p.isBot ? p.yaw : p.input.yaw) : p.yaw;
        if (!p.isBot) p.yaw = p.input.yaw;
        p.pitch = clampf(p.input.pitch, -1.3f, 1.3f);

        Vec2 wish{p.input.moveX, p.input.moveY};
        if (wish.lengthSq() > 1.0f) wish = wish.normalized();
        const ClassDef& def = classDef(p.cls);
        float speed = def.speed * (p.input.sprint ? 1.28f : 1.0f);
        speed *= (1.0f + p.upgradeLevel * 0.04f);
        map_->tryMove(p.pos, wish * speed * dt, def.radius);

        if (map_->cell(static_cast<int>(p.pos.x), static_cast<int>(p.pos.y)) == CellType::Hazard) {
            applyDamage(p, 12.0f * dt, nullptr, WeaponId::Grenade);
        }
        if (map_->cell(static_cast<int>(p.pos.x), static_cast<int>(p.pos.y)) == CellType::Health) {
            p.health = std::min(def.maxHealth, p.health + 25.0f * dt);
        }
        if (map_->cell(static_cast<int>(p.pos.x), static_cast<int>(p.pos.y)) == CellType::Ammo) {
            p.weapons[0].reserve = std::min(p.weapons[0].reserve + 1, 300);
        }

        tickWeapons(p, dt);
        if (state_ == MatchState::Playing) {
            if (p.input.reload && !p.weapons[p.weaponSlot].reloading) {
                WeaponRuntime& w = p.weapons[p.weaponSlot];
                if (w.ammoInMag < weaponDef(w.id).magazine && w.reserve > 0) {
                    w.reloading = true;
                    w.reloadLeft = weaponDef(w.id).reloadTime;
                }
            }
            if (p.input.fire) fireWeapon(p, *this, false);
            if (p.input.altFire) fireWeapon(p, *this, true);
        }

        if (def.healPerSecond > 0.0f) {
            for (auto& o : players_) {
                if (!o.active || !o.alive || o.id == p.id) continue;
                if (isTeamMode(cfg_.mode) && o.team != p.team) continue;
                if (distanceSq(o.pos, p.pos) < 4.0f * 4.0f) {
                    o.health = std::min(classDef(o.cls).maxHealth, o.health + def.healPerSecond * dt);
                }
            }
        }
        if (def.repairPerSecond > 0.0f) {
            p.armor = std::min(def.maxArmor, p.armor + def.repairPerSecond * dt * 0.25f);
        }
    }
}

void World::tickProjectiles(float dt) {
    for (auto& pr : projectiles_) {
        if (!pr.active) continue;
        pr.life -= dt;
        pr.pos += pr.vel * dt;
        bool explode = pr.life <= 0.0f;
        if (map_->isBlocked(static_cast<int>(pr.pos.x), static_cast<int>(pr.pos.y))) explode = true;
        PlayerState* hit = nullptr;
        for (auto& p : players_) {
            if (!p.active || !p.alive || p.id == pr.owner) continue;
            if (distanceSq(p.pos, pr.pos) < 0.4f * 0.4f) {
                hit = &p;
                explode = true;
                break;
            }
        }
        if (!explode) continue;
        events_.push_back({CombatEvent::Explode, pr.owner, hit ? hit->id : static_cast<uint8_t>(255), pr.pos, pr.weapon, "boom"});
        PlayerState* owner = playerById(pr.owner);
        for (auto& p : players_) {
            if (!p.active || !p.alive) continue;
            const float d = distance(p.pos, pr.pos);
            if (d <= pr.splash) {
                const float fall = 1.0f - d / std::max(0.2f, pr.splash);
                applyDamage(p, pr.damage * fall, owner, pr.weapon);
            } else if (hit == &p) {
                applyDamage(p, pr.damage, owner, pr.weapon);
            }
        }
        pr.active = false;
    }
}

void World::tickMode(float dt) {
    tickGameMode(*this, dt);
    if (state_ == MatchState::Warmup) {
        warmupLeft_ -= dt;
        if (warmupLeft_ <= 0.0f) {
            state_ = MatchState::Playing;
            scoreA_ = 0;
            scoreB_ = 0;
            for (auto& p : players_) {
                if (!p.active) continue;
                p.kills = 0;
                p.deaths = 0;
                p.score = 0;
                p.captures = 0;
                if (!p.spectating) spawnPlayer(p);
            }
            announce("Match start");
        }
        return;
    }
    if (state_ != MatchState::Playing) return;

    timeLeft_ -= dt;

    if (cfg_.mode == GameModeId::CaptureTheFlag) {
        auto tickFlag = [&](FlagState& flag, FlagState& other, TeamId team) {
            if (flag.carrier >= 0) {
                PlayerState* c = playerById(static_cast<uint8_t>(flag.carrier));
                if (!c || !c->alive) {
                    flag.carrier = -1;
                    flag.atHome = false;
                } else {
                    flag.pos = c->pos;
                    if (distance(c->pos, other.home) < 1.2f && other.atHome) {
                        c->captures += 1;
                        addScore(*c, 25);
                        if (team == TeamId::Alliance) scoreA_ += 1;
                        else scoreB_ += 1;
                        announce(std::string(c->name) + " captured the flag");
                        events_.push_back({CombatEvent::Capture, c->id, 0, c->pos, WeaponId::Grenade, "capture"});
                        flag.pos = flag.home;
                        flag.atHome = true;
                        flag.carrier = -1;
                    }
                }
            } else if (!flag.atHome) {
                // wait for pickup
            } else {
                flag.pos = flag.home;
            }
            for (auto& p : players_) {
                if (!p.active || !p.alive || p.spectating) continue;
                if (flag.carrier >= 0) break;
                if (p.team == team) {
                    if (!flag.atHome && distance(p.pos, flag.pos) < 1.0f) {
                        flag.pos = flag.home;
                        flag.atHome = true;
                    }
                } else if (distance(p.pos, flag.pos) < 1.0f) {
                    flag.carrier = p.id;
                    flag.atHome = false;
                }
            }
        };
        tickFlag(flagA_, flagB_, TeamId::Alliance);
        tickFlag(flagB_, flagA_, TeamId::Dominion);
    }

    if (cfg_.mode == GameModeId::Domination) {
        for (auto& d : dom_) {
            const int a = playersOnPoint(*this, d.pos, 1.6f, TeamId::Alliance);
            const int b = playersOnPoint(*this, d.pos, 1.6f, TeamId::Dominion);
            if (a > b) d.alliance = std::min(1.0f, d.alliance + dt * 0.35f);
            else if (b > a) d.dominion = std::min(1.0f, d.dominion + dt * 0.35f);
            if (d.alliance >= 1.0f) {
                d.owner = TeamId::Alliance;
                d.dominion = 0;
            }
            if (d.dominion >= 1.0f) {
                d.owner = TeamId::Dominion;
                d.alliance = 0;
            }
            if (d.owner == TeamId::Alliance) scoreA_ += 0; // accumulated below
        }
        static float acc = 0;
        acc += dt;
        if (acc >= 1.0f) {
            acc = 0;
            for (auto& d : dom_) {
                if (d.owner == TeamId::Alliance) scoreA_ += 1;
                if (d.owner == TeamId::Dominion) scoreB_ += 1;
            }
        }
    }

    if (cfg_.mode == GameModeId::KingOfTheHill) {
        const int a = playersOnPoint(*this, hill_, 2.0f, TeamId::Alliance);
        const int b = playersOnPoint(*this, hill_, 2.0f, TeamId::Dominion);
        if (a > b) hillOwner_ = TeamId::Alliance;
        else if (b > a) hillOwner_ = TeamId::Dominion;
        else if (a == 0 && b == 0) hillOwner_ = TeamId::None;
        if (hillOwner_ == TeamId::Alliance) scoreA_ += static_cast<int>(dt * 3.0f);
        if (hillOwner_ == TeamId::Dominion) scoreB_ += static_cast<int>(dt * 3.0f);
        // fractional accumulation
        hillHold_ += dt;
        if (hillHold_ >= 0.5f) {
            hillHold_ = 0;
            if (hillOwner_ == TeamId::Alliance) scoreA_ += 1;
            if (hillOwner_ == TeamId::Dominion) scoreB_ += 1;
        }
    }

    if (cfg_.mode == GameModeId::Deathmatch) {
        int best = 0;
        for (auto& p : players_) best = std::max(best, p.kills);
        if (rules_.scoreLimit > 0 && best >= rules_.scoreLimit) {
            state_ = MatchState::Ended;
            announce("Deathmatch complete");
        }
    } else if (rules_.scoreLimit > 0 && (scoreA_ >= rules_.scoreLimit || scoreB_ >= rules_.scoreLimit)) {
        state_ = MatchState::Ended;
        announce(scoreA_ > scoreB_ ? "SBS Alliance victory" : "Cyber Dominion victory");
    }

    if (timeLeft_ <= 0.0f && state_ == MatchState::Playing) {
        state_ = MatchState::Ended;
        announce("Time limit reached");
    }
}

void World::tick(float dt) {
    events_.clear();
    if (!map_) return;
    tick_++;
    tickPlayers(dt);
    tickProjectiles(dt);
    tickPickups(dt);
    tickMode(dt);
}

void World::buildSnapshot(std::vector<NetPlayerState>& netPlayers,
                          std::vector<NetProjectile>& netProjectiles,
                          std::vector<NetObjective>& netObjectives) const {
    netPlayers.clear();
    netProjectiles.clear();
    netObjectives.clear();
    for (const auto& p : players_) {
        if (!p.active) continue;
        NetPlayerState n{};
        n.id = p.id;
        n.flags = 0;
        if (p.alive) n.flags |= 1;
        if (p.isBot) n.flags |= 2;
        if (p.firing) n.flags |= 4;
        if (p.spectating) n.flags |= 8;
        n.classAndTeam = static_cast<uint8_t>((static_cast<uint8_t>(p.cls) & 7) | (static_cast<uint8_t>(p.team) << 3));
        n.faction = static_cast<uint8_t>(p.faction);
        n.x = quantize(p.pos.x);
        n.y = quantize(p.pos.y);
        n.yaw = yawByte(p.yaw);
        n.hp = static_cast<uint8_t>(clampf(p.health, 0, 255));
        n.weapon = static_cast<uint8_t>(p.weapons[p.weaponSlot].id);
        n.kills = static_cast<uint8_t>(clampi(p.kills, 0, 255));
        n.deaths = static_cast<uint8_t>(clampi(p.deaths, 0, 255));
        n.score = static_cast<uint8_t>(clampi(p.score, 0, 255));
        std::memcpy(n.name, p.name, kMaxNameLen);
        netPlayers.push_back(n);
    }
    for (const auto& pr : projectiles_) {
        if (!pr.active) continue;
        NetProjectile n{};
        n.id = pr.id;
        n.owner = pr.owner;
        n.weapon = static_cast<uint8_t>(pr.weapon);
        n.x = quantize(pr.pos.x);
        n.y = quantize(pr.pos.y);
        netProjectiles.push_back(n);
    }
    auto addObj = [&](uint8_t kind, TeamId owner, float progress, Vec2 pos) {
        NetObjective o{};
        o.kind = kind;
        o.owner = static_cast<uint8_t>(owner);
        o.progress = static_cast<uint8_t>(clampf(progress, 0, 1) * 255.0f);
        o.x = quantize(pos.x);
        o.y = quantize(pos.y);
        netObjectives.push_back(o);
    };
    addObj(1, flagA_.carrier >= 0 ? TeamId::Dominion : TeamId::Alliance, flagA_.atHome ? 1.0f : 0.0f, flagA_.pos);
    addObj(2, flagB_.carrier >= 0 ? TeamId::Alliance : TeamId::Dominion, flagB_.atHome ? 1.0f : 0.0f, flagB_.pos);
    addObj(3, hillOwner_, 1.0f, hill_);
    for (int i = 0; i < 3; ++i) {
        addObj(static_cast<uint8_t>(4 + i), dom_[static_cast<size_t>(i)].owner,
               std::max(dom_[static_cast<size_t>(i)].alliance, dom_[static_cast<size_t>(i)].dominion),
               dom_[static_cast<size_t>(i)].pos);
    }
}

} // namespace sbs
