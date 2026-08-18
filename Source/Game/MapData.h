#pragma once

#include "Engine/SBSTypes.h"
#include "Engine/SBSMath.h"

#include <string>
#include <vector>

namespace sbs {

struct MapTheme {
    Color fog;
    Color floorA;
    Color floorB;
    Color wallA;
    Color wallB;
    Color accent;
    float ambient = 0.55f;
    int wallTex = 0;
    int wallTexAlt = 1;
    int floorTex = 0;
};

struct SpawnPoint {
    Vec2 pos;
    TeamId team = TeamId::None;
};

struct ObjectivePoint {
    Vec2 pos;
    CellType type = CellType::Hill;
};

class GameMap {
public:
    bool loadFromFile(const std::string& path);
    bool loadBuiltin(MapId id);
    bool loadFromGrid(MapId id, const std::string& name, const MapTheme& theme,
                      int width, int height, const std::vector<char>& cells);

    int width() const { return width_; }
    int height() const { return height_; }
    MapId id() const { return id_; }
    const std::string& name() const { return name_; }
    const MapTheme& theme() const { return theme_; }

    CellType cell(int x, int y) const;
    bool inBounds(int x, int y) const;
    bool isBlocked(int x, int y) const;
    bool blocksSight(int x, int y) const;
    bool isWalkableWorld(float x, float y) const;

    Vec2 clampWorld(const Vec2& p) const;
    bool tryMove(Vec2& pos, const Vec2& delta, float radius) const;
    bool lineOfSight(const Vec2& a, const Vec2& b) const;
    bool pathfind(const Vec2& start, const Vec2& goal, std::vector<Vec2>& outPath) const;

    const std::vector<SpawnPoint>& spawns() const { return spawns_; }
    const std::vector<ObjectivePoint>& objectives() const { return objectives_; }
    Vec2 randomSpawn(TeamId team, Rng& rng) const;
    Vec2 flagPosition(TeamId team) const;
    Vec2 hillPosition() const;

    static MapTheme themeFor(MapId id);
    static std::string defaultSearchPath();

private:
    void rebuildMeta();
    int index(int x, int y) const { return y * width_ + x; }

    MapId id_ = MapId::SBSFoundry;
    std::string name_;
    MapTheme theme_{};
    int width_ = 0;
    int height_ = 0;
    std::vector<CellType> cells_;
    std::vector<SpawnPoint> spawns_;
    std::vector<ObjectivePoint> objectives_;
};

CellType charToCell(char c);
char cellToChar(CellType t);

} // namespace sbs
