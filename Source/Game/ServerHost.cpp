#include "Game/ServerHost.h"

#include "Engine/SBSLog.h"

#include <cstring>

namespace sbs {

double ServerHost::nowSeconds() const {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

bool ServerHost::start(const MatchConfig& cfg, uint16_t port, bool dedicated) {
    stop();
    cfg_ = cfg;
    dedicated_ = dedicated;
    gamePort_ = port == 0 ? kLanPort : port;
    if (!map_.loadBuiltin(cfg_.map)) {
        logError("Failed to load map");
        return false;
    }
    world_.reset(cfg_, map_);
    if (!game_.open(gamePort_, true)) return false;
    if (!discover_.open(kDiscoveryPort, true)) {
        logWarn("Discovery port busy; LAN browser replies may be limited");
    }
    info_ = ServerInfo{};
    {
        const std::string& sn = cfg_.sessionName;
        for (size_t i = 0; i + 1 < sizeof(info_.sessionName) && i < sn.size(); ++i) {
            info_.sessionName[i] = sn[i];
        }
        const std::string& mn = map_.name();
        for (size_t i = 0; i + 1 < sizeof(info_.mapName) && i < mn.size(); ++i) {
            info_.mapName[i] = mn[i];
        }
    }
    info_.mode = static_cast<uint8_t>(cfg_.mode);
    info_.maxPlayers = static_cast<uint8_t>(clampi(cfg_.maxPlayers, kMinPlayers, kMaxPlayers));
    info_.port = game_.port();
    info_.dedicated = dedicated_;
    info_.listen = !dedicated_;
    running_ = true;
    fillBots();
    logInfo(std::string(dedicated_ ? "Dedicated server" : "Listen server") + " started on port " +
            std::to_string(game_.port()) + " map=" + map_.name());
    return true;
}

void ServerHost::stop() {
    running_ = false;
    game_.close();
    discover_.close();
    clients_ = {};
}

void ServerHost::fillBots() {
    if (!cfg_.fillWithBots) return;
    const int target = clampi(cfg_.botCount, 0, cfg_.maxPlayers - 1);
    int bots = world_.botCount();
    const char* namesA[] = {"Axel","Rook","Vesper","Helix","Juno","Sable","Icarus","Nyx"};
    const char* namesB[] = {"Cipher","Nova","Kite","Zero","Lumen","Vex","Quark","Nyx-7"};
    int n = 0;
    while (bots < target && n < 64) {
        const bool alliance = (bots % 2) == 0;
        const PlayerClass cls = static_cast<PlayerClass>(bots % static_cast<int>(PlayerClass::Count));
        const char* nm = alliance ? namesA[bots % 8] : namesB[bots % 8];
        std::string name = std::string("[BOT] ") + nm;
        world_.addPlayer(name, cls, alliance ? Faction::SBSAlliance : Faction::CyberDominion, true, cfg_.difficulty);
        ++bots;
        ++n;
    }
    info_.bots = static_cast<uint8_t>(world_.botCount());
}

int ServerHost::connectedHumans() const {
    int n = 0;
    for (const auto& c : clients_) if (c.used) ++n;
    return n;
}

void ServerHost::replyDiscover(const SocketAddr& to) {
    info_.players = static_cast<uint8_t>(world_.humanCount() + world_.botCount());
    info_.bots = static_cast<uint8_t>(world_.botCount());
    uint8_t buf[512];
    const int n = packDiscoverReply(buf, sizeof(buf), info_);
    if (n > 0) {
        game_.send(to, buf, n);
        discover_.send(to, buf, n);
    }
}

void ServerHost::handlePacket(const Packet& pkt) {
    PacketType type{};
    if (!parsePacketType(pkt.bytes.data(), static_cast<int>(pkt.bytes.size()), type)) return;
    if (type == PacketType::DiscoverQuery) {
        replyDiscover(pkt.from);
        return;
    }
    if (type == PacketType::JoinRequest) {
        std::string name;
        PlayerClass cls{};
        Faction faction{};
        if (!parseJoinRequest(pkt.bytes.data(), static_cast<int>(pkt.bytes.size()), name, cls, faction)) return;
        if (world_.humanCount() >= cfg_.maxPlayers) {
            uint8_t rej[16];
            uint8_t* p = rej;
            // simple reject via JoinReject type
            const int n = packLeave(rej, sizeof(rej), 255);
            (void)p;
            if (n > 0) game_.send(pkt.from, rej, n);
            return;
        }
        const int id = world_.addPlayer(name.empty() ? "Operator" : name, cls, faction, false, cfg_.difficulty);
        if (id < 0) return;
        for (auto& c : clients_) {
            if (c.used) continue;
            c.used = true;
            c.addr = pkt.from;
            c.playerId = static_cast<uint8_t>(id);
            c.lastSeen = nowSeconds();
            break;
        }
        uint8_t buf[64];
        const int n = packJoinAccept(buf, sizeof(buf), static_cast<uint8_t>(id), cfg_.map, cfg_.mode);
        game_.send(pkt.from, buf, n);
        logInfo("Player joined: " + name + " id=" + std::to_string(id));
        return;
    }
    if (type == PacketType::ClientInput) {
        uint8_t id = 0;
        uint32_t tick = 0;
        PlayerInput in{};
        if (!parseClientInput(pkt.bytes.data(), static_cast<int>(pkt.bytes.size()), id, tick, in)) return;
        (void)tick;
        for (auto& c : clients_) {
            if (c.used && c.addr.ipv4 == pkt.from.ipv4 && c.addr.port == pkt.from.port) {
                c.lastSeen = nowSeconds();
                id = c.playerId;
                break;
            }
        }
        world_.setInput(id, in);
        return;
    }
    if (type == PacketType::Leave) {
        for (auto& c : clients_) {
            if (c.used && c.addr.ipv4 == pkt.from.ipv4 && c.addr.port == pkt.from.port) {
                world_.removePlayer(c.playerId);
                c.used = false;
                break;
            }
        }
    }
}

void ServerHost::broadcastSnapshot() {
    std::vector<NetPlayerState> ps;
    std::vector<NetProjectile> pr;
    std::vector<NetObjective> ob;
    world_.buildSnapshot(ps, pr, ob);
    uint8_t buf[4096];
    const int n = packSnapshot(buf, sizeof(buf), world_.tickIndex(), world_.state(), world_.mode(),
                               world_.timeLeft(), static_cast<int16_t>(world_.scoreA()),
                               static_cast<int16_t>(world_.scoreB()), ps, pr, ob);
    if (n <= 0) return;
    for (auto& c : clients_) {
        if (c.used) game_.send(c.addr, buf, n);
    }
}

void ServerHost::update(float dt) {
    if (!running_) return;
    Packet pkt;
    while (game_.recv(pkt)) handlePacket(pkt);
    while (discover_.recv(pkt)) handlePacket(pkt);

    const double t = nowSeconds();
    for (auto& c : clients_) {
        if (c.used && t - c.lastSeen > 8.0) {
            logWarn("Client timed out id=" + std::to_string(c.playerId));
            world_.removePlayer(c.playerId);
            c.used = false;
        }
    }

    world_.tick(dt);
    snapshotAcc_ += dt;
    if (snapshotAcc_ >= 1.0f / 20.0f) {
        snapshotAcc_ = 0.0f;
        broadcastSnapshot();
    }
    info_.players = static_cast<uint8_t>(world_.humanCount() + world_.botCount());
}

} // namespace sbs
