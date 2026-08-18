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
    float mouseSensitivity = 0.12f;
    bool invertY = false;
    int moveForward = 26; // W
    int moveBack = 22;    // S
    int moveLeft = 4;     // A
    int moveRight = 7;    // D
    int jump = 44;
    int reload = 21;
    int fire = 1;
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
