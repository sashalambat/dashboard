#pragma once

#include "Engine/SBSTypes.h"

#include <string>
#include <vector>

namespace sbs {

struct GraphicsSettings {
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    int quality = 1; // 0 low 1 high
    bool vsync = true;
    float gamma = 1.0f;
    int fpsCap = 120;
};

struct AudioSettings {
    float master = 0.8f;
    float sfx = 0.9f;
    float voice = 0.8f;
    bool positional = true;
};

struct InputSettings {
    float mouseSensitivity = 0.22f;
    bool invertY = false;
    int moveForward = 26; // SDL_SCANCODE_W
    int moveBack = 22;    // SDL_SCANCODE_S
    int moveLeft = 4;     // SDL_SCANCODE_A
    int moveRight = 7;    // SDL_SCANCODE_D
    int jump = 44;        // SDL_SCANCODE_SPACE
    int sprint = 225;     // SDL_SCANCODE_LSHIFT
    int reload = 21;      // SDL_SCANCODE_R
    int weapon1 = 30;     // SDL_SCANCODE_1
    int weapon2 = 31;     // SDL_SCANCODE_2
    int scoreboard = 43;  // SDL_SCANCODE_TAB
    int fire = -1;        // SDL_BUTTON_LEFT
    int altFire = -3;     // SDL_BUTTON_RIGHT
};

struct MatchStats {
    int matchesPlayed = 0;
    int wins = 0;
    int kills = 0;
    int deaths = 0;
    int captures = 0;
    int timePlayedSeconds = 0;
};

struct SaveData {
    std::string playerName = "Operator";
    PlayerClass preferredClass = PlayerClass::Assault;
    Faction preferredFaction = Faction::SBSAlliance;
    GraphicsSettings graphics;
    AudioSettings audio;
    InputSettings input;
    std::vector<std::string> lanFavorites;
    MatchStats stats;
};

class SaveSystem {
public:
    static std::string saveDirectory();
    static std::string savePath();
    static SaveData load();
    static bool store(const SaveData& data);
    static void addFavorite(SaveData& data, const std::string& address);
};

} // namespace sbs
