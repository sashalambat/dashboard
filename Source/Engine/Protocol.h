#pragma once

#include "Engine/SBSTypes.h"
#include "Engine/SBSMath.h"

#include <vector>

namespace sbs {

enum class PacketType : uint8_t {
    DiscoverQuery = 1,
    DiscoverReply = 2,
    JoinRequest = 3,
    JoinAccept = 4,
    JoinReject = 5,
    ClientInput = 6,
    Snapshot = 7,
    Chat = 8,
    Leave = 9,
    Heartbeat = 10
};

struct PlayerInput {
    float moveX = 0.0f;
    float moveY = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool fire = false;
    bool altFire = false;
    bool reload = false;
    bool jump = false;
    bool use = false;
    bool sprint = false;
    uint8_t weaponSlot = 0;
    uint8_t classSelect = 0;
    uint8_t factionSelect = 0;
    bool requestSpectate = false;
};

struct NetPlayerState {
    uint8_t id = 0;
    uint8_t flags = 0; // 1 alive, 2 bot, 4 firing, 8 scoped
    uint8_t classAndTeam = 0;
    uint8_t faction = 0;
    int16_t x = 0;
    int16_t y = 0;
    uint8_t yaw = 0;
    uint8_t hp = 0;
    uint8_t weapon = 0;
    uint8_t kills = 0;
    uint8_t deaths = 0;
    uint8_t score = 0;
    char name[kMaxNameLen]{};
};

struct NetProjectile {
    uint8_t id = 0;
    uint8_t owner = 0;
    uint8_t weapon = 0;
    int16_t x = 0;
    int16_t y = 0;
};

struct NetObjective {
    uint8_t kind = 0;
    uint8_t owner = 0;
    uint8_t progress = 0;
    int16_t x = 0;
    int16_t y = 0;
};

struct ServerInfo {
    char sessionName[48]{};
    char mapName[40]{};
    uint8_t mode = 0;
    uint8_t players = 0;
    uint8_t maxPlayers = kMaxPlayers;
    uint8_t bots = 0;
    uint16_t port = kLanPort;
    bool dedicated = false;
    bool listen = true;
};

int packDiscoverQuery(uint8_t* out, int cap);
int packDiscoverReply(uint8_t* out, int cap, const ServerInfo& info);
int packJoinRequest(uint8_t* out, int cap, const char* name, PlayerClass cls, Faction faction);
int packJoinAccept(uint8_t* out, int cap, uint8_t playerId, MapId map, GameModeId mode);
int packClientInput(uint8_t* out, int cap, uint8_t playerId, uint32_t tick, const PlayerInput& in);
int packLeave(uint8_t* out, int cap, uint8_t playerId);

bool parsePacketType(const uint8_t* data, int len, PacketType& type);
bool parseDiscoverReply(const uint8_t* data, int len, ServerInfo& info);
bool parseJoinRequest(const uint8_t* data, int len, std::string& name, PlayerClass& cls, Faction& faction);
bool parseJoinAccept(const uint8_t* data, int len, uint8_t& playerId, MapId& map, GameModeId& mode);
bool parseClientInput(const uint8_t* data, int len, uint8_t& playerId, uint32_t& tick, PlayerInput& in);

int packSnapshot(uint8_t* out, int cap, uint32_t tick, MatchState state, GameModeId mode,
                 float timeLeft, int16_t scoreA, int16_t scoreB,
                 const std::vector<NetPlayerState>& players,
                 const std::vector<NetProjectile>& projectiles,
                 const std::vector<NetObjective>& objectives);

bool parseSnapshot(const uint8_t* data, int len, uint32_t& tick, MatchState& state, GameModeId& mode,
                   float& timeLeft, int16_t& scoreA, int16_t& scoreB,
                   std::vector<NetPlayerState>& players,
                   std::vector<NetProjectile>& projectiles,
                   std::vector<NetObjective>& objectives);

} // namespace sbs
