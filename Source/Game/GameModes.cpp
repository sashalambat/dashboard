#include "Game/GameModes.h"

#include "Game/World.h"

namespace sbs {

const char* modeShortName(GameModeId id) {
    switch (id) {
        case GameModeId::Deathmatch: return "DM";
        case GameModeId::TeamDeathmatch: return "TDM";
        case GameModeId::CaptureTheFlag: return "CTF";
        case GameModeId::Domination: return "DOM";
        case GameModeId::KingOfTheHill: return "KOTH";
        default: return "???";
    }
}

int playersOnPoint(const World& world, const Vec2& pos, float radius, TeamId team) {
    int n = 0;
    for (const auto& p : world.players()) {
        if (!p.active || !p.alive || p.spectating) continue;
        if (team != TeamId::None && p.team != team) continue;
        if (distanceSq(p.pos, pos) <= radius * radius) ++n;
    }
    return n;
}

void tickGameMode(World& world, float dt) {
    (void)world;
    (void)dt;
}

} // namespace sbs
