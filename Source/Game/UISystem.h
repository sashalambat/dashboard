#pragma once

#include "Engine/Renderer.h"
#include "Engine/Protocol.h"
#include "Engine/NetSocket.h"
#include "Game/SaveSystem.h"
#include "Game/World.h"

namespace sbs {

enum class UiScreen {
    MainMenu,
    Options,
    Settings,
    LanBrowser,
    ClassSelect,
    SpawnMenu,
    Playing,
    Scoreboard,
    Spectator,
    Results,
    Statistics
};

struct LanEntry {
    ServerInfo info;
    SocketAddr addr;
    float age = 0.0f;
};

class UISystem {
public:
    void draw(Renderer& r, UiScreen screen, const SaveData& save, const World* world, uint8_t localId,
              const std::vector<LanEntry>& servers, int cursor, const std::string& status) const;
    int menuCount(UiScreen screen) const;
};

} // namespace sbs
