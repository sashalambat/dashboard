#include "Engine/Protocol.h"

#include <cstring>

namespace sbs {
namespace {

bool writeU8(uint8_t*& p, const uint8_t* end, uint8_t v) {
    if (p >= end) return false;
    *p++ = v;
    return true;
}

bool writeU16(uint8_t*& p, const uint8_t* end, uint16_t v) {
    if (end - p < 2) return false;
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    p += 2;
    return true;
}

bool writeU32(uint8_t*& p, const uint8_t* end, uint32_t v) {
    if (end - p < 4) return false;
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFF);
    p += 4;
    return true;
}

bool writeI16(uint8_t*& p, const uint8_t* end, int16_t v) {
    return writeU16(p, end, static_cast<uint16_t>(v));
}

bool writeBytes(uint8_t*& p, const uint8_t* end, const void* src, int n) {
    if (end - p < n) return false;
    std::memcpy(p, src, static_cast<size_t>(n));
    p += n;
    return true;
}

bool readU8(const uint8_t*& p, const uint8_t* end, uint8_t& v) {
    if (p >= end) return false;
    v = *p++;
    return true;
}

bool readU16(const uint8_t*& p, const uint8_t* end, uint16_t& v) {
    if (end - p < 2) return false;
    v = static_cast<uint16_t>(p[0] | (p[1] << 8));
    p += 2;
    return true;
}

bool readU32(const uint8_t*& p, const uint8_t* end, uint32_t& v) {
    if (end - p < 4) return false;
    v = static_cast<uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
    p += 4;
    return true;
}

bool readI16(const uint8_t*& p, const uint8_t* end, int16_t& v) {
    uint16_t u = 0;
    if (!readU16(p, end, u)) return false;
    v = static_cast<int16_t>(u);
    return true;
}

bool readBytes(const uint8_t*& p, const uint8_t* end, void* dst, int n) {
    if (end - p < n) return false;
    std::memcpy(dst, p, static_cast<size_t>(n));
    p += n;
    return true;
}

int beginPacket(uint8_t* out, int cap, PacketType type, uint8_t*& p, const uint8_t*& end) {
    if (!out || cap < 8) return 0;
    p = out;
    end = out + cap;
    if (!writeU32(p, end, kProtocolMagic)) return 0;
    if (!writeU16(p, end, kProtocolVersion)) return 0;
    if (!writeU8(p, end, static_cast<uint8_t>(type))) return 0;
    return 1;
}

bool checkHeader(const uint8_t*& p, const uint8_t* end, PacketType expected) {
    uint32_t magic = 0;
    uint16_t ver = 0;
    uint8_t type = 0;
    if (!readU32(p, end, magic) || magic != kProtocolMagic) return false;
    if (!readU16(p, end, ver) || ver != kProtocolVersion) return false;
    if (!readU8(p, end, type) || type != static_cast<uint8_t>(expected)) return false;
    return true;
}

void writeFixedString(uint8_t*& p, const uint8_t* end, const char* s, int n) {
    if (!p || end - p < n || n <= 0) return;
    std::memset(p, 0, static_cast<size_t>(n));
    if (s) {
        int i = 0;
        while (i < n - 1 && s[i] != '\0') {
            p[i] = static_cast<uint8_t>(s[i]);
            ++i;
        }
    }
    p += n;
}

} // namespace

int packDiscoverQuery(uint8_t* out, int cap) {
    uint8_t* p = nullptr;
    const uint8_t* end = nullptr;
    if (!beginPacket(out, cap, PacketType::DiscoverQuery, p, end)) return 0;
    return static_cast<int>(p - out);
}

int packDiscoverReply(uint8_t* out, int cap, const ServerInfo& info) {
    uint8_t* p = nullptr;
    const uint8_t* end = nullptr;
    if (!beginPacket(out, cap, PacketType::DiscoverReply, p, end)) return 0;
    writeFixedString(p, end, info.sessionName, 48);
    writeFixedString(p, end, info.mapName, 40);
    writeU8(p, end, info.mode);
    writeU8(p, end, info.players);
    writeU8(p, end, info.maxPlayers);
    writeU8(p, end, info.bots);
    writeU16(p, end, info.port);
    writeU8(p, end, info.dedicated ? 1 : 0);
    writeU8(p, end, info.listen ? 1 : 0);
    return static_cast<int>(p - out);
}

int packJoinRequest(uint8_t* out, int cap, const char* name, PlayerClass cls, Faction faction) {
    uint8_t* p = nullptr;
    const uint8_t* end = nullptr;
    if (!beginPacket(out, cap, PacketType::JoinRequest, p, end)) return 0;
    char tmp[kMaxNameLen];
    std::memset(tmp, 0, sizeof(tmp));
    if (name) {
        size_t i = 0;
        while (i + 1 < kMaxNameLen && name[i] != 0) {
            tmp[i] = name[i];
            ++i;
        }
    }
    writeBytes(p, end, tmp, kMaxNameLen);
    writeU8(p, end, static_cast<uint8_t>(cls));
    writeU8(p, end, static_cast<uint8_t>(faction));
    return static_cast<int>(p - out);
}

int packJoinAccept(uint8_t* out, int cap, uint8_t playerId, MapId map, GameModeId mode) {
    uint8_t* p = nullptr;
    const uint8_t* end = nullptr;
    if (!beginPacket(out, cap, PacketType::JoinAccept, p, end)) return 0;
    writeU8(p, end, playerId);
    writeU8(p, end, static_cast<uint8_t>(map));
    writeU8(p, end, static_cast<uint8_t>(mode));
    return static_cast<int>(p - out);
}

int packClientInput(uint8_t* out, int cap, uint8_t playerId, uint32_t tick, const PlayerInput& in) {
    uint8_t* p = nullptr;
    const uint8_t* end = nullptr;
    if (!beginPacket(out, cap, PacketType::ClientInput, p, end)) return 0;
    writeU8(p, end, playerId);
    writeU32(p, end, tick);
    writeI16(p, end, static_cast<int16_t>(in.moveX * 1000.0f));
    writeI16(p, end, static_cast<int16_t>(in.moveY * 1000.0f));
    writeI16(p, end, static_cast<int16_t>(in.yaw * 1000.0f));
    writeI16(p, end, static_cast<int16_t>(in.pitch * 1000.0f));
    uint8_t flags = 0;
    if (in.fire) flags |= 1;
    if (in.altFire) flags |= 2;
    if (in.reload) flags |= 4;
    if (in.jump) flags |= 8;
    if (in.use) flags |= 16;
    if (in.sprint) flags |= 32;
    if (in.requestSpectate) flags |= 64;
    writeU8(p, end, flags);
    writeU8(p, end, in.weaponSlot);
    writeU8(p, end, in.classSelect);
    writeU8(p, end, in.factionSelect);
    return static_cast<int>(p - out);
}

int packLeave(uint8_t* out, int cap, uint8_t playerId) {
    uint8_t* p = nullptr;
    const uint8_t* end = nullptr;
    if (!beginPacket(out, cap, PacketType::Leave, p, end)) return 0;
    writeU8(p, end, playerId);
    return static_cast<int>(p - out);
}

bool parsePacketType(const uint8_t* data, int len, PacketType& type) {
    if (!data || len < 7) return false;
    uint32_t magic = 0;
    uint16_t ver = 0;
    uint8_t t = 0;
    const uint8_t* p = data;
    const uint8_t* end = data + len;
    if (!readU32(p, end, magic) || magic != kProtocolMagic) return false;
    if (!readU16(p, end, ver) || ver != kProtocolVersion) return false;
    if (!readU8(p, end, t)) return false;
    type = static_cast<PacketType>(t);
    return true;
}

bool parseDiscoverReply(const uint8_t* data, int len, ServerInfo& info) {
    const uint8_t* p = data;
    const uint8_t* end = data + len;
    if (!checkHeader(p, end, PacketType::DiscoverReply)) return false;
    if (!readBytes(p, end, info.sessionName, 48)) return false;
    if (!readBytes(p, end, info.mapName, 40)) return false;
    uint8_t dedicated = 0, listen = 0;
    if (!readU8(p, end, info.mode)) return false;
    if (!readU8(p, end, info.players)) return false;
    if (!readU8(p, end, info.maxPlayers)) return false;
    if (!readU8(p, end, info.bots)) return false;
    if (!readU16(p, end, info.port)) return false;
    if (!readU8(p, end, dedicated)) return false;
    if (!readU8(p, end, listen)) return false;
    info.sessionName[47] = 0;
    info.mapName[39] = 0;
    info.dedicated = dedicated != 0;
    info.listen = listen != 0;
    return true;
}

bool parseJoinRequest(const uint8_t* data, int len, std::string& name, PlayerClass& cls, Faction& faction) {
    const uint8_t* p = data;
    const uint8_t* end = data + len;
    if (!checkHeader(p, end, PacketType::JoinRequest)) return false;
    char tmp[kMaxNameLen];
    if (!readBytes(p, end, tmp, kMaxNameLen)) return false;
    tmp[kMaxNameLen - 1] = 0;
    name = tmp;
    uint8_t c = 0, f = 0;
    if (!readU8(p, end, c) || !readU8(p, end, f)) return false;
    cls = static_cast<PlayerClass>(c);
    faction = static_cast<Faction>(f);
    return true;
}

bool parseJoinAccept(const uint8_t* data, int len, uint8_t& playerId, MapId& map, GameModeId& mode) {
    const uint8_t* p = data;
    const uint8_t* end = data + len;
    if (!checkHeader(p, end, PacketType::JoinAccept)) return false;
    uint8_t m = 0, g = 0;
    if (!readU8(p, end, playerId) || !readU8(p, end, m) || !readU8(p, end, g)) return false;
    map = static_cast<MapId>(m);
    mode = static_cast<GameModeId>(g);
    return true;
}

bool parseClientInput(const uint8_t* data, int len, uint8_t& playerId, uint32_t& tick, PlayerInput& in) {
    const uint8_t* p = data;
    const uint8_t* end = data + len;
    if (!checkHeader(p, end, PacketType::ClientInput)) return false;
    if (!readU8(p, end, playerId) || !readU32(p, end, tick)) return false;
    int16_t mx = 0, my = 0, yaw = 0, pitch = 0;
    uint8_t flags = 0;
    if (!readI16(p, end, mx) || !readI16(p, end, my) || !readI16(p, end, yaw) || !readI16(p, end, pitch)) return false;
    if (!readU8(p, end, flags)) return false;
    if (!readU8(p, end, in.weaponSlot) || !readU8(p, end, in.classSelect) || !readU8(p, end, in.factionSelect)) return false;
    in.moveX = mx / 1000.0f;
    in.moveY = my / 1000.0f;
    in.yaw = yaw / 1000.0f;
    in.pitch = pitch / 1000.0f;
    in.fire = (flags & 1) != 0;
    in.altFire = (flags & 2) != 0;
    in.reload = (flags & 4) != 0;
    in.jump = (flags & 8) != 0;
    in.use = (flags & 16) != 0;
    in.sprint = (flags & 32) != 0;
    in.requestSpectate = (flags & 64) != 0;
    return true;
}

int packSnapshot(uint8_t* out, int cap, uint32_t tick, MatchState state, GameModeId mode,
                 float timeLeft, int16_t scoreA, int16_t scoreB,
                 const std::vector<NetPlayerState>& players,
                 const std::vector<NetProjectile>& projectiles,
                 const std::vector<NetObjective>& objectives) {
    uint8_t* p = nullptr;
    const uint8_t* end = nullptr;
    if (!beginPacket(out, cap, PacketType::Snapshot, p, end)) return 0;
    writeU32(p, end, tick);
    writeU8(p, end, static_cast<uint8_t>(state));
    writeU8(p, end, static_cast<uint8_t>(mode));
    writeU16(p, end, static_cast<uint16_t>(clampf(timeLeft, 0.0f, 65535.0f)));
    writeI16(p, end, scoreA);
    writeI16(p, end, scoreB);
    writeU8(p, end, static_cast<uint8_t>(std::min<size_t>(players.size(), 64)));
    writeU8(p, end, static_cast<uint8_t>(std::min<size_t>(projectiles.size(), 64)));
    writeU8(p, end, static_cast<uint8_t>(std::min<size_t>(objectives.size(), 8)));
    const size_t pc = std::min<size_t>(players.size(), 64);
    for (size_t i = 0; i < pc; ++i) {
        const NetPlayerState& pl = players[i];
        writeU8(p, end, pl.id);
        writeU8(p, end, pl.flags);
        writeU8(p, end, pl.classAndTeam);
        writeU8(p, end, pl.faction);
        writeI16(p, end, pl.x);
        writeI16(p, end, pl.y);
        writeU8(p, end, pl.yaw);
        writeU8(p, end, pl.hp);
        writeU8(p, end, pl.weapon);
        writeU8(p, end, pl.kills);
        writeU8(p, end, pl.deaths);
        writeU8(p, end, pl.score);
        writeBytes(p, end, pl.name, kMaxNameLen);
    }
    const size_t prc = std::min<size_t>(projectiles.size(), 64);
    for (size_t i = 0; i < prc; ++i) {
        const NetProjectile& pr = projectiles[i];
        writeU8(p, end, pr.id);
        writeU8(p, end, pr.owner);
        writeU8(p, end, pr.weapon);
        writeI16(p, end, pr.x);
        writeI16(p, end, pr.y);
    }
    const size_t oc = std::min<size_t>(objectives.size(), 8);
    for (size_t i = 0; i < oc; ++i) {
        const NetObjective& o = objectives[i];
        writeU8(p, end, o.kind);
        writeU8(p, end, o.owner);
        writeU8(p, end, o.progress);
        writeI16(p, end, o.x);
        writeI16(p, end, o.y);
    }
    return static_cast<int>(p - out);
}

bool parseSnapshot(const uint8_t* data, int len, uint32_t& tick, MatchState& state, GameModeId& mode,
                   float& timeLeft, int16_t& scoreA, int16_t& scoreB,
                   std::vector<NetPlayerState>& players,
                   std::vector<NetProjectile>& projectiles,
                   std::vector<NetObjective>& objectives) {
    const uint8_t* p = data;
    const uint8_t* end = data + len;
    if (!checkHeader(p, end, PacketType::Snapshot)) return false;
    uint8_t st = 0, md = 0, pc = 0, prc = 0, oc = 0;
    uint16_t tl = 0;
    if (!readU32(p, end, tick)) return false;
    if (!readU8(p, end, st) || !readU8(p, end, md) || !readU16(p, end, tl)) return false;
    if (!readI16(p, end, scoreA) || !readI16(p, end, scoreB)) return false;
    if (!readU8(p, end, pc) || !readU8(p, end, prc) || !readU8(p, end, oc)) return false;
    state = static_cast<MatchState>(st);
    mode = static_cast<GameModeId>(md);
    timeLeft = static_cast<float>(tl);
    players.clear();
    projectiles.clear();
    objectives.clear();
    players.reserve(pc);
    for (uint8_t i = 0; i < pc; ++i) {
        NetPlayerState pl{};
        if (!readU8(p, end, pl.id) || !readU8(p, end, pl.flags) || !readU8(p, end, pl.classAndTeam) || !readU8(p, end, pl.faction)) return false;
        if (!readI16(p, end, pl.x) || !readI16(p, end, pl.y) || !readU8(p, end, pl.yaw) || !readU8(p, end, pl.hp)) return false;
        if (!readU8(p, end, pl.weapon) || !readU8(p, end, pl.kills) || !readU8(p, end, pl.deaths) || !readU8(p, end, pl.score)) return false;
        if (!readBytes(p, end, pl.name, kMaxNameLen)) return false;
        pl.name[kMaxNameLen - 1] = 0;
        players.push_back(pl);
    }
    for (uint8_t i = 0; i < prc; ++i) {
        NetProjectile pr{};
        if (!readU8(p, end, pr.id) || !readU8(p, end, pr.owner) || !readU8(p, end, pr.weapon)) return false;
        if (!readI16(p, end, pr.x) || !readI16(p, end, pr.y)) return false;
        projectiles.push_back(pr);
    }
    for (uint8_t i = 0; i < oc; ++i) {
        NetObjective o{};
        if (!readU8(p, end, o.kind) || !readU8(p, end, o.owner) || !readU8(p, end, o.progress)) return false;
        if (!readI16(p, end, o.x) || !readI16(p, end, o.y)) return false;
        objectives.push_back(o);
    }
    return true;
}

} // namespace sbs
