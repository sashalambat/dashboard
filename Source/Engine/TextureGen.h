#pragma once

#include "Engine/SBSMath.h"

#include <vector>

namespace sbs {

std::vector<Color> generateWallTexture(uint32_t seed, Color a, Color b, int size);
std::vector<Color> generateFloorTexture(uint32_t seed, Color a, Color b, int size);

} // namespace sbs
