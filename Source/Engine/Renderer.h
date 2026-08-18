#pragma once

#include "Engine/SBSMath.h"
#include "Engine/SBSTypes.h"
#include "Game/World.h"
#include "Game/SaveSystem.h"

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
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
    int width() const { return outputW_; }
    int height() const { return outputH_; }
    SDL_Renderer* sdl() const { return renderer_; }
    void setGamma(float g) { gamma_ = g; }

    void drawWeapon(const PlayerState& p, float bob);
    void drawCrosshair(bool hit);

private:
    uint32_t pack(Color c) const;
    void put(int x, int y, Color c);
    void renderColumn(const World& world, const PlayerState& view, int x);
    void renderSprites(const World& world, const PlayerState& view);

    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    ViewFrame frame_;
    int outputW_ = 1280;
    int outputH_ = 720;
    float gamma_ = 1.0f;
    int quality_ = 1;
    std::vector<Color> wallTex_;
    std::vector<Color> wallTexB_;
};

} // namespace sbs
