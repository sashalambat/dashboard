#include "Engine/TextureGen.h"

namespace sbs {

std::vector<Color> generateWallTexture(uint32_t seed, Color a, Color b, int size) {
    Rng rng(seed);
    std::vector<Color> tex(static_cast<size_t>(size * size));
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float n = rng.nextFloat();
            Color c;
            c.r = static_cast<uint8_t>(lerpf(a.r, b.r, n));
            c.g = static_cast<uint8_t>(lerpf(a.g, b.g, n));
            c.b = static_cast<uint8_t>(lerpf(a.b, b.b, n));
            c.a = 255;
            if (x == 0 || y == 0 || x == size - 1 || y == size - 1) {
                c.r = static_cast<uint8_t>(c.r * 0.55f);
                c.g = static_cast<uint8_t>(c.g * 0.55f);
                c.b = static_cast<uint8_t>(c.b * 0.55f);
            }
            if ((x % 8) == 0 || (y % 8) == 0) {
                c.r = static_cast<uint8_t>(std::min(255, c.r + 18));
                c.g = static_cast<uint8_t>(std::min(255, c.g + 12));
            }
            tex[static_cast<size_t>(y * size + x)] = c;
        }
    }
    return tex;
}

std::vector<Color> generateFloorTexture(uint32_t seed, Color a, Color b, int size) {
    return generateWallTexture(seed ^ 0xA5A5u, a, b, size);
}

} // namespace sbs
