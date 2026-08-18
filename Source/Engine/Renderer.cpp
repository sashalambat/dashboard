#include "Engine/Renderer.h"

#include "Engine/TextureGen.h"

#include <cmath>
#include <cstring>

namespace sbs {
namespace {

// 5x7 bitmap font for 95 printable ASCII chars starting at 32.
const uint8_t kFont[96][7] = {
    {0,0,0,0,0,0,0}, // space
    {4,4,4,4,0,4,0}, {10,10,0,0,0,0,0}, {10,31,10,31,10,0,0}, {4,14,20,14,5,14,4},
    {17,18,4,8,19,0,0}, {8,20,8,21,18,13,0}, {4,4,0,0,0,0,0}, {2,4,4,4,4,2,0},
    {8,4,4,4,4,8,0}, {0,10,4,31,4,10,0}, {0,4,4,31,4,4,0}, {0,0,0,0,4,4,8},
    {0,0,0,31,0,0,0}, {0,0,0,0,0,4,0}, {1,2,4,8,16,0,0}, {14,17,19,21,25,14,0},
    {4,12,4,4,4,14,0}, {14,17,2,4,8,31,0}, {14,17,6,1,17,14,0}, {2,6,10,18,31,2,0},
    {31,16,30,1,17,14,0}, {6,8,30,17,17,14,0}, {31,1,2,4,8,8,0}, {14,17,14,17,17,14,0},
    {14,17,17,15,1,14,0}, {0,4,0,0,4,0,0}, {0,4,0,0,4,8,0}, {2,4,8,4,2,0,0},
    {0,0,31,0,31,0,0}, {8,4,2,4,8,0,0}, {14,17,2,4,0,4,0}, {14,17,21,23,16,14,0},
    {14,17,17,31,17,17,0}, {30,17,30,17,17,30,0}, {14,17,16,16,17,14,0},
    {30,17,17,17,17,30,0}, {31,16,30,16,16,31,0}, {31,16,30,16,16,16,0},
    {14,17,16,19,17,15,0}, {17,17,31,17,17,17,0}, {14,4,4,4,4,14,0},
    {1,1,1,1,17,14,0}, {17,18,28,18,17,17,0}, {16,16,16,16,16,31,0},
    {17,27,21,17,17,17,0}, {17,25,21,19,17,17,0}, {14,17,17,17,17,14,0},
    {30,17,17,30,16,16,0}, {14,17,17,21,18,13,0}, {30,17,17,30,18,17,0},
    {14,17,14,1,17,14,0}, {31,4,4,4,4,4,0}, {17,17,17,17,17,14,0},
    {17,17,17,17,10,4,0}, {17,17,17,21,21,10,0}, {17,17,14,17,17,17,0},
    {17,17,10,4,4,4,0}, {31,1,2,4,8,31,0}, {14,8,8,8,8,14,0}, {16,8,4,2,1,0,0},
    {14,2,2,2,2,14,0}, {4,10,17,0,0,0,0}, {0,0,0,0,0,31,0}, {8,4,0,0,0,0,0},
    {0,14,1,15,17,15,0}, {16,16,30,17,17,30,0}, {0,14,16,16,16,14,0},
    {1,1,15,17,17,15,0}, {0,14,17,31,16,14,0}, {6,8,28,8,8,8,0},
    {0,15,17,15,1,14,0}, {16,16,30,17,17,17,0}, {4,0,12,4,4,14,0},
    {2,0,2,2,18,12,0}, {16,18,20,24,20,18,0}, {12,4,4,4,4,14,0},
    {0,26,21,21,21,21,0}, {0,30,17,17,17,17,0}, {0,14,17,17,17,14,0},
    {0,30,17,30,16,16,0}, {0,15,17,15,1,1,0}, {0,22,24,16,16,16,0},
    {0,15,16,14,1,30,0}, {8,28,8,8,8,6,0}, {0,17,17,17,17,15,0},
    {0,17,17,17,10,4,0}, {0,17,17,21,21,10,0}, {0,17,10,4,10,17,0},
    {0,17,17,15,1,14,0}, {0,31,2,4,8,31,0}, {6,8,8,16,8,8,6}, {4,4,4,4,4,4,4},
    {12,2,2,1,2,2,12}, {0,8,21,2,0,0,0}, {31,31,31,31,31,31,31}
};

int glyphIndex(char c) {
    const unsigned char uc = static_cast<unsigned char>(c);
    if (uc < 32 || uc > 127) return 0;
    return uc - 32;
}

} // namespace

bool Renderer::init(SDL_Window* window, const GraphicsSettings& gfx) {
    outputW_ = gfx.width;
    outputH_ = gfx.height;
    quality_ = gfx.quality;
    gamma_ = gfx.gamma;
    renderer_ = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | (gfx.vsync ? SDL_RENDERER_PRESENTVSYNC : 0));
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window, -1, 0);
    }
    if (!renderer_) return false;
    frame_.width = (quality_ >= 1) ? 480 : 320;
    frame_.height = (quality_ >= 1) ? 270 : 180;
    frame_.pixels.assign(static_cast<size_t>(frame_.width * frame_.height), 0);
    frame_.depth.assign(static_cast<size_t>(frame_.width), 0.0f);
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                 frame_.width, frame_.height);
    wallTex_ = generateWallTexture(0x1111, Color::rgb(90, 70, 50), Color::rgb(40, 30, 20), 64);
    wallTexB_ = generateWallTexture(0x2222, Color::rgb(50, 80, 90), Color::rgb(20, 30, 40), 64);
    return texture_ != nullptr;
}

void Renderer::shutdown() {
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    texture_ = nullptr;
    renderer_ = nullptr;
}

uint32_t Renderer::pack(Color c) const {
    auto g = [&](uint8_t v) {
        float f = std::pow(v / 255.0f, 1.0f / std::max(0.4f, gamma_));
        return static_cast<uint8_t>(clampf(f * 255.0f, 0, 255));
    };
    return (255u << 24) | (static_cast<uint32_t>(g(c.r)) << 16) | (static_cast<uint32_t>(g(c.g)) << 8) | g(c.b);
}

void Renderer::put(int x, int y, Color c) {
    if (x < 0 || y < 0 || x >= frame_.width || y >= frame_.height) return;
    frame_.pixels[static_cast<size_t>(y * frame_.width + x)] = pack(c);
}

void Renderer::renderColumn(const World& world, const PlayerState& view, int x) {
    const float cameraX = 2.0f * static_cast<float>(x) / static_cast<float>(frame_.width) - 1.0f;
    const Vec2 dir = dirFromYaw(view.yaw);
    const Vec2 plane{-dir.y * 0.66f, dir.x * 0.66f};
    const Vec2 ray{dir.x + plane.x * cameraX, dir.y + plane.y * cameraX};
    int mapX = static_cast<int>(view.pos.x);
    int mapY = static_cast<int>(view.pos.y);
    const float deltaX = (ray.x == 0.0f) ? 1e30f : std::fabs(1.0f / ray.x);
    const float deltaY = (ray.y == 0.0f) ? 1e30f : std::fabs(1.0f / ray.y);
    int stepX, stepY;
    float sideX, sideY;
    if (ray.x < 0) {
        stepX = -1;
        sideX = (view.pos.x - mapX) * deltaX;
    } else {
        stepX = 1;
        sideX = (mapX + 1.0f - view.pos.x) * deltaX;
    }
    if (ray.y < 0) {
        stepY = -1;
        sideY = (view.pos.y - mapY) * deltaY;
    } else {
        stepY = 1;
        sideY = (mapY + 1.0f - view.pos.y) * deltaY;
    }
    int side = 0;
    int hits = 0;
    while (hits++ < 64) {
        if (sideX < sideY) {
            sideX += deltaX;
            mapX += stepX;
            side = 0;
        } else {
            sideY += deltaY;
            mapY += stepY;
            side = 1;
        }
        if (world.map().isBlocked(mapX, mapY) || world.map().cell(mapX, mapY) == CellType::Window) break;
    }
    float dist;
    if (side == 0) dist = (sideX - deltaX);
    else dist = (sideY - deltaY);
    dist = std::max(0.05f, dist);
    frame_.depth[static_cast<size_t>(x)] = dist;
    const int lineH = static_cast<int>(frame_.height / dist);
    int drawStart = std::max(0, -lineH / 2 + frame_.height / 2);
    int drawEnd = std::min(frame_.height - 1, lineH / 2 + frame_.height / 2);
    const MapTheme& th = world.map().theme();
    float wallX = (side == 0) ? (view.pos.y + dist * ray.y) : (view.pos.x + dist * ray.x);
    wallX -= std::floor(wallX);
    int texX = static_cast<int>(wallX * 64.0f) & 63;
    const float fog = clampf(dist / 28.0f, 0.0f, 1.0f);
    for (int y = 0; y < frame_.height; ++y) {
        Color c;
        if (y < drawStart) {
            const float t = static_cast<float>(y) / frame_.height;
            c = Color::rgb(static_cast<uint8_t>(th.fog.r * (0.4f + t)),
                           static_cast<uint8_t>(th.fog.g * (0.4f + t)),
                           static_cast<uint8_t>(th.fog.b * (0.6f + t)));
        } else if (y > drawEnd) {
            const bool check = (((static_cast<int>(view.pos.x + dir.x * (frame_.height / static_cast<float>(y - frame_.height / 2))) +
                                  static_cast<int>(view.pos.y + dir.y)) & 1) != 0);
            c = check ? th.floorA : th.floorB;
        } else {
            const int d = y * 256 - frame_.height * 128 + lineH * 128;
            int texY = ((d * 64) / lineH / 256) & 63;
            const std::vector<Color>& tex = (side == 1) ? wallTexB_ : wallTex_;
            c = tex[static_cast<size_t>(texY * 64 + texX)];
            if (side == 1) {
                c.r = static_cast<uint8_t>(c.r * 0.72f);
                c.g = static_cast<uint8_t>(c.g * 0.72f);
                c.b = static_cast<uint8_t>(c.b * 0.72f);
            }
            c.r = static_cast<uint8_t>(lerpf(c.r, th.fog.r, fog));
            c.g = static_cast<uint8_t>(lerpf(c.g, th.fog.g, fog));
            c.b = static_cast<uint8_t>(lerpf(c.b, th.fog.b, fog));
        }
        put(x, y, c);
    }
}

void Renderer::renderSprites(const World& world, const PlayerState& view) {
    const Vec2 dir = dirFromYaw(view.yaw);
    const Vec2 plane{-dir.y * 0.66f, dir.x * 0.66f};
    const float invDet = 1.0f / (plane.x * dir.y - dir.x * plane.y);
    for (const auto& p : world.players()) {
        if (!p.active || p.id == view.id || (!p.alive && !p.spectating)) continue;
        if (!p.alive) continue;
        const Vec2 rel = p.pos - view.pos;
        const float tx = invDet * (dir.y * rel.x - dir.x * rel.y);
        const float ty = invDet * (-plane.y * rel.x + plane.x * rel.y);
        if (ty <= 0.05f) continue;
        const int screenX = static_cast<int>((frame_.width / 2) * (1.0f + tx / ty));
        const int spriteH = std::abs(static_cast<int>(frame_.height / ty));
        const int spriteW = spriteH / 2;
        Color body = factionColor(p.faction);
        for (int sx = -spriteW / 2; sx < spriteW / 2; ++sx) {
            const int x = screenX + sx;
            if (x < 0 || x >= frame_.width) continue;
            if (ty > frame_.depth[static_cast<size_t>(x)]) continue;
            for (int sy = 0; sy < spriteH; ++sy) {
                const int y = frame_.height / 2 - spriteH / 2 + sy;
                Color c = body;
                if (sy < spriteH / 5) c = Color::rgb(30, 30, 30);
                if (p.firing && sy > spriteH / 3 && sy < spriteH / 2) c = Color::rgb(255, 220, 80);
                put(x, y, c);
            }
        }
    }
}

void Renderer::drawWeapon(const PlayerState& p, float bob) {
    const int w = frame_.width;
    const int h = frame_.height;
    const int ox = w / 2 - 40 + static_cast<int>(std::sin(bob) * 6.0f);
    const int oy = h - 70 + static_cast<int>(std::fabs(std::cos(bob)) * 4.0f) + static_cast<int>(p.weapons[p.weaponSlot].recoilPitch * 80.0f);
    Color metal = (p.faction == Faction::CyberDominion) ? Color::rgb(40, 180, 200) : Color::rgb(180, 120, 50);
    for (int y = 0; y < 55; ++y) {
        for (int x = 0; x < 80; ++x) {
            if (x > 20 && y > 18 && x < 70 && y < 32) put(ox + x, oy + y, metal);
            if (x > 60 && y > 20 && x < 78 && y < 28) put(ox + x, oy + y, Color::rgb(20, 20, 20));
            if (p.firing && x > 74 && y > 21 && y < 27) put(ox + x, oy + y, Color::rgb(255, 200, 60));
        }
    }
}

void Renderer::drawCrosshair(bool hit) {
    const Color c = hit ? Color::rgb(255, 80, 80) : Color::rgb(240, 240, 240);
    const int cx = frame_.width / 2;
    const int cy = frame_.height / 2;
    for (int i = -4; i <= 4; ++i) {
        if (i != 0) {
            put(cx + i, cy, c);
            put(cx, cy + i, c);
        }
    }
}

void Renderer::renderMinimap(const World& world, uint8_t localId) {
    const int scale = 3;
    const int mw = world.map().width() * scale;
    const int mh = world.map().height() * scale;
    const int ox = 8;
    const int oy = 8;
    for (int y = 0; y < world.map().height(); ++y) {
        for (int x = 0; x < world.map().width(); ++x) {
            Color c = Color::rgb(20, 20, 20);
            if (!world.map().isBlocked(x, y)) c = Color::rgb(50, 50, 50);
            const CellType t = world.map().cell(x, y);
            if (t == CellType::Hill) c = Color::rgb(200, 180, 40);
            if (t == CellType::FlagA) c = Color::rgb(255, 140, 40);
            if (t == CellType::FlagB) c = Color::rgb(40, 200, 255);
            for (int py = 0; py < scale; ++py)
                for (int px = 0; px < scale; ++px)
                    put(ox + x * scale + px, oy + y * scale + py, c);
        }
    }
    (void)mw;
    (void)mh;
    for (const auto& p : world.players()) {
        if (!p.active || !p.alive) continue;
        const int x = ox + static_cast<int>(p.pos.x * scale);
        const int y = oy + static_cast<int>(p.pos.y * scale);
        put(x, y, p.id == localId ? Color::rgb(255, 255, 255) : factionColor(p.faction));
    }
}

void Renderer::renderWorld(const World& world, uint8_t localId, bool spectator) {
    std::fill(frame_.pixels.begin(), frame_.pixels.end(), pack(world.map().theme().fog));
    const PlayerState* view = world.playerById(localId);
    PlayerState cam{};
    if (view) cam = *view;
    else {
        cam.pos = {world.map().width() * 0.5f, world.map().height() * 0.5f};
        cam.yaw = 0;
    }
    if (spectator || (view && view->spectating) || (view && !view->alive)) {
        for (const auto& p : world.players()) {
            if (p.active && p.alive) {
                cam.pos = p.pos;
                cam.yaw = p.yaw;
                break;
            }
        }
    }
    for (int x = 0; x < frame_.width; ++x) renderColumn(world, cam, x);
    renderSprites(world, cam);
    if (view && view->alive && !view->spectating) {
        const float bob = view->pos.x * 3.0f + view->pos.y * 3.0f;
        drawWeapon(*view, bob);
        drawCrosshair(false);
    }
    renderMinimap(world, localId);
}

void Renderer::present() {
    if (!renderer_ || !texture_) return;
    SDL_UpdateTexture(texture_, nullptr, frame_.pixels.data(), frame_.width * static_cast<int>(sizeof(uint32_t)));
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

void Renderer::drawRect(int x, int y, int w, int h, Color c) {
    for (int yy = 0; yy < h; ++yy)
        for (int xx = 0; xx < w; ++xx)
            put(x + xx, y + yy, c);
}

void Renderer::drawText(int x, int y, const std::string& text, Color c, int scale) {
    int cx = x;
    for (char ch : text) {
        if (ch == '\n') {
            y += 8 * scale;
            cx = x;
            continue;
        }
        const uint8_t* g = kFont[glyphIndex(ch)];
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (g[row] & (16 >> col)) {
                    for (int sy = 0; sy < scale; ++sy)
                        for (int sx = 0; sx < scale; ++sx)
                            put(cx + col * scale + sx, y + row * scale + sy, c);
                }
            }
        }
        cx += 6 * scale;
    }
}

void Renderer::drawTextCenter(int y, const std::string& text, Color c, int scale) {
    const int w = static_cast<int>(text.size()) * 6 * scale;
    drawText((frame_.width - w) / 2, y, text, c, scale);
}

} // namespace sbs
