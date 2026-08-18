#pragma once

#include "Engine/SBSTypes.h"
#include "Engine/SBSMath.h"

namespace sbs {

class World;

const char* modeShortName(GameModeId id);
int playersOnPoint(const World& world, const Vec2& pos, float radius, TeamId team);
void tickGameMode(World& world, float dt);

} // namespace sbs
