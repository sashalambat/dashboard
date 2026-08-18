#include "Game/SaveSystem.h"

#include "Engine/SBSLog.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#if defined(_WIN32)
#include <direct.h>
#endif

namespace sbs {
namespace {

void ensureDir(const std::string& path) {
#if defined(_WIN32)
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

std::string get(const std::string& body, const std::string& key, const std::string& def) {
    const std::string token = key + "=";
    const auto pos = body.find(token);
    if (pos == std::string::npos) return def;
    auto end = body.find('\n', pos);
    if (end == std::string::npos) end = body.size();
    return body.substr(pos + token.size(), end - (pos + token.size()));
}

int geti(const std::string& body, const std::string& key, int def) {
    try {
        return std::stoi(get(body, key, std::to_string(def)));
    } catch (...) {
        return def;
    }
}

float getf(const std::string& body, const std::string& key, float def) {
    try {
        return std::stof(get(body, key, std::to_string(def)));
    } catch (...) {
        return def;
    }
}

} // namespace

std::string SaveSystem::saveDirectory() {
#if defined(_WIN32)
    const char* home = std::getenv("APPDATA");
    std::string dir = home ? std::string(home) + "\\SBSWars" : std::string(".sbswars");
#else
    const char* home = std::getenv("HOME");
    std::string dir = home ? std::string(home) + "/.sbswars" : std::string(".sbswars");
#endif
    ensureDir(dir);
    return dir;
}

std::string SaveSystem::savePath() {
#if defined(_WIN32)
    return saveDirectory() + "\\settings.ini";
#else
    return saveDirectory() + "/settings.ini";
#endif
}

SaveData SaveSystem::load() {
    SaveData d;
    std::ifstream in(savePath());
    if (!in) return d;
    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string body = ss.str();
    d.playerName = get(body, "name", d.playerName);
    d.preferredClass = static_cast<PlayerClass>(geti(body, "class", static_cast<int>(d.preferredClass)));
    d.preferredFaction = static_cast<Faction>(geti(body, "faction", static_cast<int>(d.preferredFaction)));
    d.graphics.width = geti(body, "width", d.graphics.width);
    d.graphics.height = geti(body, "height", d.graphics.height);
    d.graphics.fullscreen = geti(body, "fullscreen", d.graphics.fullscreen ? 1 : 0) != 0;
    d.graphics.quality = geti(body, "quality", d.graphics.quality);
    d.graphics.vsync = geti(body, "vsync", d.graphics.vsync ? 1 : 0) != 0;
    d.graphics.gamma = getf(body, "gamma", d.graphics.gamma);
    d.graphics.fpsCap = geti(body, "fpsCap", d.graphics.fpsCap);
    d.audio.master = getf(body, "master", d.audio.master);
    d.audio.sfx = getf(body, "sfx", d.audio.sfx);
    d.audio.voice = getf(body, "voice", d.audio.voice);
    d.audio.positional = geti(body, "positional", d.audio.positional ? 1 : 0) != 0;
    d.input.mouseSensitivity = getf(body, "sens", d.input.mouseSensitivity);
    d.input.invertY = geti(body, "invertY", d.input.invertY ? 1 : 0) != 0;
    d.input.moveForward = geti(body, "bindForward", d.input.moveForward);
    d.input.moveBack = geti(body, "bindBack", d.input.moveBack);
    d.input.moveLeft = geti(body, "bindLeft", d.input.moveLeft);
    d.input.moveRight = geti(body, "bindRight", d.input.moveRight);
    d.input.jump = geti(body, "bindJump", d.input.jump);
    d.input.sprint = geti(body, "bindSprint", d.input.sprint);
    d.input.reload = geti(body, "bindReload", d.input.reload);
    d.input.weapon1 = geti(body, "bindWeapon1", d.input.weapon1);
    d.input.weapon2 = geti(body, "bindWeapon2", d.input.weapon2);
    d.input.scoreboard = geti(body, "bindScoreboard", d.input.scoreboard);
    d.input.fire = geti(body, "bindFire", d.input.fire);
    d.input.altFire = geti(body, "bindAltFire", d.input.altFire);
    d.stats.matchesPlayed = geti(body, "matches", 0);
    d.stats.wins = geti(body, "wins", 0);
    d.stats.kills = geti(body, "kills", 0);
    d.stats.deaths = geti(body, "deaths", 0);
    d.stats.captures = geti(body, "captures", 0);
    d.stats.timePlayedSeconds = geti(body, "time", 0);
    const std::string fav = get(body, "favorites", "");
    if (!fav.empty()) {
        std::stringstream fs(fav);
        std::string item;
        while (std::getline(fs, item, ';')) {
            if (!item.empty()) d.lanFavorites.push_back(item);
        }
    }
    return d;
}

bool SaveSystem::store(const SaveData& data) {
    std::ofstream out(savePath(), std::ios::trunc);
    if (!out) {
        logError("Unable to write save file " + savePath());
        return false;
    }
    std::string fav;
    for (size_t i = 0; i < data.lanFavorites.size(); ++i) {
        if (i) fav += ";";
        fav += data.lanFavorites[i];
    }
    out << "name=" << data.playerName << "\n";
    out << "class=" << static_cast<int>(data.preferredClass) << "\n";
    out << "faction=" << static_cast<int>(data.preferredFaction) << "\n";
    out << "width=" << data.graphics.width << "\n";
    out << "height=" << data.graphics.height << "\n";
    out << "fullscreen=" << (data.graphics.fullscreen ? 1 : 0) << "\n";
    out << "quality=" << data.graphics.quality << "\n";
    out << "vsync=" << (data.graphics.vsync ? 1 : 0) << "\n";
    out << "gamma=" << data.graphics.gamma << "\n";
    out << "fpsCap=" << data.graphics.fpsCap << "\n";
    out << "master=" << data.audio.master << "\n";
    out << "sfx=" << data.audio.sfx << "\n";
    out << "voice=" << data.audio.voice << "\n";
    out << "positional=" << (data.audio.positional ? 1 : 0) << "\n";
    out << "sens=" << data.input.mouseSensitivity << "\n";
    out << "invertY=" << (data.input.invertY ? 1 : 0) << "\n";
    out << "bindForward=" << data.input.moveForward << "\n";
    out << "bindBack=" << data.input.moveBack << "\n";
    out << "bindLeft=" << data.input.moveLeft << "\n";
    out << "bindRight=" << data.input.moveRight << "\n";
    out << "bindJump=" << data.input.jump << "\n";
    out << "bindSprint=" << data.input.sprint << "\n";
    out << "bindReload=" << data.input.reload << "\n";
    out << "bindWeapon1=" << data.input.weapon1 << "\n";
    out << "bindWeapon2=" << data.input.weapon2 << "\n";
    out << "bindScoreboard=" << data.input.scoreboard << "\n";
    out << "bindFire=" << data.input.fire << "\n";
    out << "bindAltFire=" << data.input.altFire << "\n";
    out << "matches=" << data.stats.matchesPlayed << "\n";
    out << "wins=" << data.stats.wins << "\n";
    out << "kills=" << data.stats.kills << "\n";
    out << "deaths=" << data.stats.deaths << "\n";
    out << "captures=" << data.stats.captures << "\n";
    out << "time=" << data.stats.timePlayedSeconds << "\n";
    out << "favorites=" << fav << "\n";
    return true;
}

void SaveSystem::addFavorite(SaveData& data, const std::string& address) {
    if (std::find(data.lanFavorites.begin(), data.lanFavorites.end(), address) == data.lanFavorites.end()) {
        data.lanFavorites.push_back(address);
    }
}

} // namespace sbs
