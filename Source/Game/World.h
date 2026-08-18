#pragma once

#include "Game/WeaponSystem.h"
#include "Game/MapData.h"

#include <functional>

namespace sbs {

struct MatchConfig {
    GameModeId mode = GameModeId::TeamDeathmatch;
    MapId map = MapId::SBSFoundry;
    int maxPlayers = 16;
    int botCount = 8;
    BotDifficulty difficulty = BotDifficulty::Veteran;
    bool fillWithBots = true;
    bool dedicated = false;
    bool listenServer = true;
    float timeLimit = 0.0f;
    int scoreLimit = 0;
    int warmupSeconds = -1;
    std::string sessionName = "SBS Wars LAN";
};

class World {
public:
    void reset(const MatchConfig& cfg, const GameMap& map);
    void tick(float dt);
    void setInput(uint8_t id, const PlayerInput& input);

    int addPlayer(const std::string& name, PlayerClass cls, Faction faction, bool bot, BotDifficulty diff);
    void removePlayer(uint8_t id);
    void spawnPlayer(PlayerState& p);
    void killPlayer(PlayerState& victim, PlayerState* killer, WeaponId weapon);
    void applyDamage(PlayerState& victim, float amount, PlayerState* src, WeaponId weapon);
    void addProjectile(const PlayerState& owner, WeaponId weapon, const Vec2& origin, const Vec2& dir);

    PlayerState* playerById(uint8_t id);
    const PlayerState* playerById(uint8_t id) const;
    PlayerState* closestEnemy(const PlayerState& from, float maxDist, bool requireLos);

    const GameMap& map() const { return *map_; }
    const MatchConfig& config() const { return cfg_; }
    MatchState state() const { return state_; }
    GameModeId mode() const { return cfg_.mode; }
    float timeLeft() const { return timeLeft_; }
    int scoreA() const { return scoreA_; }
    int scoreB() const { return scoreB_; }
    uint32_t tickIndex() const { return tick_; }
    const std::array<PlayerState, kMaxPlayers>& players() const { return players_; }
    std::array<PlayerState, kMaxPlayers>& players() { return players_; }
    const std::array<ProjectileState, kMaxProjectiles>& projectiles() const { return projectiles_; }
    const FlagState& flagA() const { return flagA_; }
    const FlagState& flagB() const { return flagB_; }
    const std::array<DomPoint, 3>& dom() const { return dom_; }
    Vec2 hill() const { return hill_; }
    TeamId hillOwner() const { return hillOwner_; }
    const std::vector<CombatEvent>& events() const { return events_; }
    const char* lastAnnouncement() const { return lastAnnounce_.c_str(); }
    const std::array<WeaponPickup, 16>& pickups() const { return pickups_; }
    int humanCount() const;
    int botCount() const;
    int aliveCount() const;
    bool matchOver() const { return state_ == MatchState::Ended; }

    void buildSnapshot(std::vector<NetPlayerState>& players,
                       std::vector<NetProjectile>& projectiles,
                       std::vector<NetObjective>& objectives) const;
    void announce(const std::string& text);
    void addScore(PlayerState& p, int amount);

    Rng& rng() { return rng_; }

private:
    void tickPlayers(float dt);
    void tickProjectiles(float dt);
    void tickMode(float dt);
    void tickPickups(float dt);
    void placePickups();
    void hitscan(PlayerState& attacker, const WeaponDef& def, const Vec2& origin, float yaw);

    MatchConfig cfg_{};
    const GameMap* map_ = nullptr;
    GameMap ownedMap_;
    MatchState state_ = MatchState::Lobby;
    ModeRules rules_{};
    uint32_t tick_ = 0;
    float timeLeft_ = 0.0f;
    float warmupLeft_ = 0.0f;
    int scoreA_ = 0;
    int scoreB_ = 0;
    std::array<PlayerState, kMaxPlayers> players_{};
    std::array<ProjectileState, kMaxProjectiles> projectiles_{};
    FlagState flagA_{};
    FlagState flagB_{};
    std::array<DomPoint, 3> dom_{};
    Vec2 hill_{};
    TeamId hillOwner_ = TeamId::None;
    float hillHold_ = 0.0f;
    std::vector<CombatEvent> events_;
    std::string lastAnnounce_;
    std::array<WeaponPickup, 16> pickups_{};
    Rng rng_{0x5B5A};
};

void tickBot(World& world, PlayerState& bot, float dt);
void tickGameMode(World& world, float dt);

} // namespace sbs
