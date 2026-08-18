#pragma once

#include "Engine/SBSMath.h"
#include "Engine/SBSTypes.h"
#include "Engine/Bitmap.h"
#include "Game/World.h"
#include "Game/SaveSystem.h"

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#include <array>
#include <vector>

namespace sbs {

struct ViewFrame {
    int width = 960;
    int height = 540;
    std::vector<uint32_t> pixels;
    std::vector<float> depth;
};

class Renderer {
public:
    bool init(SDL_Window* window, const GraphicsSettings& gfx);
    void shutdown();
    void renderWorld(const World& world, uint8_t localId, bool spectator);
    void renderMinimap(const World& world, uint8_t localId);
    void present();
    void drawRect(int x, int y, int w, int h, Color c);
    void drawText(int x, int y, const std::string& text, Color c, int scale = 1);
    void drawTextCenter(int y, const std::string& text, Color c, int scale = 2);
    int frameWidth() const { return frame_.width; }
    int frameHeight() const { return frame_.height; }
    int width() const { return outputW_; }
    int height() const { return outputH_; }
    SDL_Renderer* sdl() const { return renderer_; }
    void setGamma(float g) { gamma_ = g; }

    void drawWeapon(const PlayerState& p, float bob);
    void drawCrosshair(bool hit);

private:
    uint32_t pack(Color c) const;
    void put(int x, int y, Color c);
    void putMasked(int x, int y, Color c);
    void blit(const Bitmap& bmp, int dx, int dy, int dw, int dh, float shade = 1.0f);
    void renderColumn(const World& world, const PlayerState& view, int x);
    void renderSprites(const World& world, const PlayerState& view);
    const Bitmap& wallTex(const World& world, int mx, int my, int side) const;
    const Bitmap& charSprite(const PlayerState& p) const;
    void loadArt();

    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    ViewFrame frame_;
    int outputW_ = 1280;
    int outputH_ = 720;
    float gamma_ = 1.0f;
    int quality_ = 1;
    std::vector<Color> wallTex_;
    std::vector<Color> wallTexB_;
    std::array<Bitmap, 16> walls_{};
    std::array<Bitmap, 8> floors_{};
    std::array<Bitmap, 9> weapons_{};
    std::array<Bitmap, 8> pickups_{};
    Bitmap sky_{};
    Bitmap crosshair_{};
    Bitmap burst_{};
    Bitmap charAlliance_{};
    Bitmap charAllianceFire_{};
    Bitmap charDominion_{};
    Bitmap charDominionFire_{};
    Bitmap charEnemy_{};
    Bitmap charDrone_{};
};

} // namespace sbs
