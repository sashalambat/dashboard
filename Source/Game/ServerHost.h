#pragma once

#include "Engine/NetSocket.h"
#include "Engine/Protocol.h"
#include "Game/World.h"

#include <chrono>
#include <string>
#include <array>

namespace sbs {

struct ClientSlot {
    bool used = false;
    SocketAddr addr;
    uint8_t playerId = 0;
    double lastSeen = 0.0;
};

class ServerHost {
public:
    bool start(const MatchConfig& cfg, uint16_t port, bool dedicated);
    void stop();
    void update(float dt);
    void fillBots();

    World& world() { return world_; }
    const World& world() const { return world_; }
    bool running() const { return running_; }
    const ServerInfo& info() const { return info_; }
    uint16_t port() const { return gamePort_; }
    bool dedicated() const { return dedicated_; }

    int connectedHumans() const;

private:
    void handlePacket(const Packet& pkt);
    void broadcastSnapshot();
    void replyDiscover(const SocketAddr& to);
    double nowSeconds() const;

    World world_;
    GameMap map_;
    MatchConfig cfg_{};
    ServerInfo info_{};
    UdpChannel game_;
    UdpChannel discover_;
    std::array<ClientSlot, kMaxPlayers> clients_{};
    uint16_t gamePort_ = kLanPort;
    bool running_ = false;
    bool dedicated_ = false;
    float snapshotAcc_ = 0.0f;
};

} // namespace sbs
