#include "Game/MapData.h"

#include "Engine/SBSLog.h"

#include <fstream>
#include <functional>
#include <queue>

namespace sbs {
namespace {

CellType decode(char c) {
    switch (c) {
        case '#': return CellType::Wall;
        case '+': return CellType::Cover;
        case 'A': return CellType::SpawnA;
        case 'a': return CellType::SpawnA;
        case 'B': return CellType::SpawnB;
        case 'b': return CellType::SpawnB;
        case 'F': return CellType::FlagA;
        case 'G': return CellType::FlagB;
        case '1': return CellType::Dom1;
        case '2': return CellType::Dom2;
        case '3': return CellType::Dom3;
        case 'K': return CellType::Hill;
        case 'H': return CellType::Health;
        case 'W': return CellType::Ammo;
        case 'X': return CellType::Window;
        case '~': return CellType::Hazard;
        default: return CellType::Empty;
    }
}

void fillRect(std::vector<char>& g, int w, int h, int x0, int y0, int x1, int y1, char c) {
    for (int y = y0; y <= y1 && y < h; ++y) {
        for (int x = x0; x <= x1 && x < w; ++x) {
            if (x >= 0 && y >= 0) g[static_cast<size_t>(y * w + x)] = c;
        }
    }
}

void stamp(std::vector<char>& g, int w, int h, int x, int y, char c) {
    if (x >= 0 && y >= 0 && x < w && y < h) g[static_cast<size_t>(y * w + x)] = c;
}

void border(std::vector<char>& g, int w, int h) {
    fillRect(g, w, h, 0, 0, w - 1, 0, '#');
    fillRect(g, w, h, 0, h - 1, w - 1, h - 1, '#');
    fillRect(g, w, h, 0, 0, 0, h - 1, '#');
    fillRect(g, w, h, w - 1, 0, w - 1, h - 1, '#');
}

std::vector<char> blank(int w, int h) {
    return std::vector<char>(static_cast<size_t>(w * h), '.');
}

std::string mapNameForFile(const std::string& path) {
    const auto slash = path.find_last_of("/\\");
    std::string base = slash == std::string::npos ? path : path.substr(slash + 1);
    const auto dot = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);
    return base;
}

void rooms(std::vector<char>& g, int w, int h, int step) {
    for (int y = 6; y < h - 6; y += step) {
        for (int x = 6; x < w - 6; x += step) {
            fillRect(g, w, h, x, y, std::min(w - 2, x + 3), y, '#');
            fillRect(g, w, h, x, y, x, std::min(h - 2, y + 3), '#');
        }
    }
}

} // namespace

CellType charToCell(char c) { return decode(c); }

char cellToChar(CellType t) {
    switch (t) {
        case CellType::Wall: return '#';
        case CellType::Cover: return '+';
        case CellType::SpawnA: return 'A';
        case CellType::SpawnB: return 'B';
        case CellType::FlagA: return 'F';
        case CellType::FlagB: return 'G';
        case CellType::Dom1: return '1';
        case CellType::Dom2: return '2';
        case CellType::Dom3: return '3';
        case CellType::Hill: return 'K';
        case CellType::Health: return 'H';
        case CellType::Ammo: return 'W';
        case CellType::Hazard: return '~';
        case CellType::Window: return 'X';
        default: return '.';
    }
}

MapTheme GameMap::themeFor(MapId id) {
    switch (id) {
        case MapId::SBSFoundry:
            return {Color::rgb(40, 28, 18), Color::rgb(70, 55, 40), Color::rgb(50, 40, 30),
                    Color::rgb(110, 80, 50), Color::rgb(80, 60, 40), Color::rgb(255, 140, 40), 0.6f, 0, 10, 0};
        case MapId::ArcticBaseZeta:
            return {Color::rgb(20, 40, 55), Color::rgb(180, 200, 220), Color::rgb(140, 170, 200),
                    Color::rgb(90, 130, 160), Color::rgb(200, 220, 230), Color::rgb(80, 180, 255), 0.7f, 11, 12, 2};
        case MapId::OrbitalPlatformSeven:
            return {Color::rgb(8, 8, 18), Color::rgb(30, 32, 48), Color::rgb(18, 20, 32),
                    Color::rgb(70, 80, 110), Color::rgb(40, 50, 80), Color::rgb(120, 180, 255), 0.45f, 2, 14, 1};
        case MapId::CrimsonDesert:
            return {Color::rgb(60, 25, 15), Color::rgb(170, 90, 40), Color::rgb(140, 70, 30),
                    Color::rgb(120, 50, 25), Color::rgb(90, 40, 20), Color::rgb(255, 80, 40), 0.75f, 13, 4, 5};
        case MapId::CyberCore:
            return {Color::rgb(10, 20, 18), Color::rgb(20, 40, 36), Color::rgb(12, 28, 24),
                    Color::rgb(20, 80, 70), Color::rgb(10, 50, 45), Color::rgb(40, 255, 200), 0.5f, 6, 15, 3};
        case MapId::TitanFactory:
        default:
            return {Color::rgb(25, 22, 20), Color::rgb(60, 58, 54), Color::rgb(40, 38, 36),
                    Color::rgb(90, 88, 80), Color::rgb(50, 48, 44), Color::rgb(255, 200, 60), 0.55f, 1, 10, 0};
    }
}

std::string GameMap::defaultSearchPath() {
    return "Maps";
}

bool GameMap::inBounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

CellType GameMap::cell(int x, int y) const {
    if (!inBounds(x, y) || cells_.empty()) return CellType::Wall;
    return cells_[static_cast<size_t>(index(x, y))];
}

bool GameMap::isBlocked(int x, int y) const {
    const CellType t = cell(x, y);
    return t == CellType::Wall || t == CellType::Cover || t == CellType::Window;
}

bool GameMap::blocksSight(int x, int y) const {
    const CellType t = cell(x, y);
    return t == CellType::Wall || t == CellType::Cover;
}

bool GameMap::isWalkableWorld(float x, float y) const {
    return !isBlocked(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)));
}

Vec2 GameMap::clampWorld(const Vec2& p) const {
    return {clampf(p.x, 1.2f, static_cast<float>(width_) - 1.2f),
            clampf(p.y, 1.2f, static_cast<float>(height_) - 1.2f)};
}

bool GameMap::tryMove(Vec2& pos, const Vec2& delta, float radius) const {
    Vec2 next = pos + delta;
    auto blocked = [&](float x, float y) {
        return isBlocked(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)));
    };
    if (!blocked(next.x, pos.y) && !blocked(next.x - radius, pos.y) && !blocked(next.x + radius, pos.y)) {
        pos.x = next.x;
    }
    if (!blocked(pos.x, next.y) && !blocked(pos.x, next.y - radius) && !blocked(pos.x, next.y + radius)) {
        pos.y = next.y;
    }
    pos = clampWorld(pos);
    return true;
}

bool GameMap::lineOfSight(const Vec2& a, const Vec2& b) const {
    float x = a.x;
    float y = a.y;
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(dx), std::fabs(dy)) * 8.0f)));
    const float sx = dx / static_cast<float>(steps);
    const float sy = dy / static_cast<float>(steps);
    for (int i = 0; i < steps; ++i) {
        x += sx;
        y += sy;
        if (blocksSight(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)))) {
            return false;
        }
    }
    return true;
}

bool GameMap::pathfind(const Vec2& start, const Vec2& goal, std::vector<Vec2>& outPath) const {
    outPath.clear();
    const int sx = clampi(static_cast<int>(start.x), 0, width_ - 1);
    const int sy = clampi(static_cast<int>(start.y), 0, height_ - 1);
    const int gx = clampi(static_cast<int>(goal.x), 0, width_ - 1);
    const int gy = clampi(static_cast<int>(goal.y), 0, height_ - 1);
    if (isBlocked(gx, gy)) return false;

    auto key = [&](int x, int y) { return y * width_ + x; };
    const int total = width_ * height_;
    std::vector<int> came(static_cast<size_t>(total), -1);
    std::vector<int> gScore(static_cast<size_t>(total), 1'000'000);
    auto h = [&](int x, int y) { return std::abs(x - gx) + std::abs(y - gy); };

    using Node = std::pair<int, int>; // f, key
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    const int startK = key(sx, sy);
    gScore[static_cast<size_t>(startK)] = 0;
    open.push({h(sx, sy), startK});

    const int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    bool found = false;
    int guard = 0;
    while (!open.empty() && guard++ < 8000) {
        const int current = open.top().second;
        open.pop();
        if (current == key(gx, gy)) {
            found = true;
            break;
        }
        const int cx = current % width_;
        const int cy = current / width_;
        for (auto& d : dirs) {
            const int nx = cx + d[0];
            const int ny = cy + d[1];
            if (!inBounds(nx, ny) || isBlocked(nx, ny)) continue;
            if (d[0] != 0 && d[1] != 0 && (isBlocked(cx + d[0], cy) || isBlocked(cx, cy + d[1]))) continue;
            const int nk = key(nx, ny);
            const int cost = gScore[static_cast<size_t>(current)] + ((d[0] != 0 && d[1] != 0) ? 14 : 10);
            if (cost < gScore[static_cast<size_t>(nk)]) {
                came[static_cast<size_t>(nk)] = current;
                gScore[static_cast<size_t>(nk)] = cost;
                open.push({cost + h(nx, ny) * 10, nk});
            }
        }
    }
    if (!found) return false;
    int cur = key(gx, gy);
    std::vector<Vec2> rev;
    while (cur != -1) {
        rev.push_back({static_cast<float>(cur % width_) + 0.5f, static_cast<float>(cur / width_) + 0.5f});
        if (cur == startK) break;
        cur = came[static_cast<size_t>(cur)];
    }
    outPath.assign(rev.rbegin(), rev.rend());
    return !outPath.empty();
}

void GameMap::rebuildMeta() {
    spawns_.clear();
    objectives_.clear();
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const CellType t = cell(x, y);
            const Vec2 p{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
            if (t == CellType::SpawnA) spawns_.push_back({p, TeamId::Alliance});
            if (t == CellType::SpawnB) spawns_.push_back({p, TeamId::Dominion});
            if (t == CellType::FlagA || t == CellType::FlagB || t == CellType::Dom1 ||
                t == CellType::Dom2 || t == CellType::Dom3 || t == CellType::Hill) {
                objectives_.push_back({p, t});
            }
        }
    }
}

Vec2 GameMap::randomSpawn(TeamId team, Rng& rng) const {
    std::vector<Vec2> opts;
    for (const auto& s : spawns_) {
        if (team == TeamId::None || team == TeamId::Spectator || s.team == team || !isTeamMode(GameModeId::TeamDeathmatch)) {
            opts.push_back(s.pos);
        }
    }
    if (opts.empty()) {
        for (const auto& s : spawns_) opts.push_back(s.pos);
    }
    if (opts.empty()) {
        return {width_ * 0.5f, height_ * 0.5f};
    }
    return opts[static_cast<size_t>(rng.rangeInt(0, static_cast<int>(opts.size()) - 1))];
}

Vec2 GameMap::flagPosition(TeamId team) const {
    for (const auto& o : objectives_) {
        if (team == TeamId::Alliance && o.type == CellType::FlagA) return o.pos;
        if (team == TeamId::Dominion && o.type == CellType::FlagB) return o.pos;
    }
    return {width_ * 0.5f, height_ * 0.5f};
}

Vec2 GameMap::hillPosition() const {
    for (const auto& o : objectives_) {
        if (o.type == CellType::Hill) return o.pos;
    }
    return {width_ * 0.5f, height_ * 0.5f};
}

bool GameMap::loadFromGrid(MapId id, const std::string& name, const MapTheme& theme,
                           int width, int height, const std::vector<char>& cells) {
    if (width <= 4 || height <= 4 || static_cast<int>(cells.size()) != width * height) {
        return false;
    }
    id_ = id;
    name_ = name;
    theme_ = theme;
    width_ = width;
    height_ = height;
    cells_.assign(static_cast<size_t>(width * height), CellType::Empty);
    for (int i = 0; i < width * height; ++i) {
        cells_[static_cast<size_t>(i)] = decode(cells[static_cast<size_t>(i)]);
    }
    rebuildMeta();
    return true;
}

bool GameMap::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::string magic;
    in >> magic;
    int version = 0;
    in >> version;
    if (magic != "SBSMAP") return false;
    std::string key;
    std::string mapName;
    std::string themeName;
    int w = 0, h = 0;
    while (in >> key) {
        if (key == "name:") {
            std::getline(in, mapName);
            if (!mapName.empty() && mapName[0] == ' ') mapName.erase(0, 1);
        } else if (key == "theme:") {
            in >> themeName;
        } else if (key == "width:") {
            in >> w;
        } else if (key == "height:") {
            in >> h;
        } else if (key == "grid:") {
            break;
        }
    }
    if (w <= 0 || h <= 0) return false;
    std::vector<char> grid(static_cast<size_t>(w * h), '.');
    std::string line;
    std::getline(in, line);
    for (int y = 0; y < h; ++y) {
        if (!std::getline(in, line)) break;
        for (int x = 0; x < w && x < static_cast<int>(line.size()); ++x) {
            grid[static_cast<size_t>(y * w + x)] = line[static_cast<size_t>(x)];
        }
    }
    MapId id = MapId::SBSFoundry;
    if (themeName == "arctic") id = MapId::ArcticBaseZeta;
    else if (themeName == "orbital") id = MapId::OrbitalPlatformSeven;
    else if (themeName == "desert") id = MapId::CrimsonDesert;
    else if (themeName == "cyber") id = MapId::CyberCore;
    else if (themeName == "titan") id = MapId::TitanFactory;
    return loadFromGrid(id, mapName.empty() ? mapNameForFile(path) : mapName, themeFor(id), w, h, grid);
}

bool GameMap::loadBuiltin(MapId id) {
    const int w = 40;
    const int h = 40;
    auto g = blank(w, h);
    border(g, w, h);
    rooms(g, w, h, 8);

    auto placeObjectives = [&]() {
        stamp(g, w, h, 4, 4, 'A');
        stamp(g, w, h, 5, 4, 'A');
        stamp(g, w, h, 4, 5, 'a');
        stamp(g, w, h, 5, 5, 'a');
        stamp(g, w, h, w - 6, h - 6, 'B');
        stamp(g, w, h, w - 5, h - 6, 'B');
        stamp(g, w, h, w - 6, h - 5, 'b');
        stamp(g, w, h, w - 5, h - 5, 'b');
        stamp(g, w, h, 6, 6, 'F');
        stamp(g, w, h, w - 7, h - 7, 'G');
        stamp(g, w, h, 12, 20, '1');
        stamp(g, w, h, 20, 12, '2');
        stamp(g, w, h, 28, 28, '3');
        stamp(g, w, h, 20, 20, 'K');
        stamp(g, w, h, 10, 10, 'H');
        stamp(g, w, h, 30, 30, 'W');
    };

    switch (id) {
        case MapId::SBSFoundry:
            fillRect(g, w, h, 10, 8, 16, 8, '#');
            fillRect(g, w, h, 22, 18, 30, 18, '#');
            fillRect(g, w, h, 8, 24, 8, 32, '#');
            stamp(g, w, h, 18, 18, '+');
            stamp(g, w, h, 19, 22, '+');
            break;
        case MapId::ArcticBaseZeta:
            fillRect(g, w, h, 15, 4, 24, 6, '#');
            fillRect(g, w, h, 4, 16, 10, 22, '#');
            fillRect(g, w, h, 28, 14, 35, 20, '#');
            for (int i = 8; i < 32; i += 4) stamp(g, w, h, i, 18, '+');
            break;
        case MapId::OrbitalPlatformSeven:
            fillRect(g, w, h, 12, 12, 27, 27, '.');
            fillRect(g, w, h, 18, 1, 21, 38, '.');
            fillRect(g, w, h, 1, 18, 38, 21, '.');
            fillRect(g, w, h, 17, 17, 22, 22, '#');
            stamp(g, w, h, 17, 20, '.');
            stamp(g, w, h, 22, 20, '.');
            break;
        case MapId::CrimsonDesert:
            for (int i = 5; i < 35; i += 5) {
                fillRect(g, w, h, i, 10, i + 1, 12, '#');
                fillRect(g, w, h, i + 2, 26, i + 3, 28, '#');
            }
            fillRect(g, w, h, 16, 16, 24, 24, '~');
            fillRect(g, w, h, 18, 18, 22, 22, '.');
            break;
        case MapId::CyberCore:
            for (int y = 8; y < 32; y += 6) {
                for (int x = 8; x < 32; x += 6) {
                    fillRect(g, w, h, x, y, x + 2, y + 2, '#');
                    stamp(g, w, h, x + 1, y, 'X');
                }
            }
            break;
        case MapId::TitanFactory:
            fillRect(g, w, h, 6, 10, 34, 12, '#');
            fillRect(g, w, h, 6, 26, 34, 28, '#');
            stamp(g, w, h, 12, 11, '.');
            stamp(g, w, h, 20, 11, '.');
            stamp(g, w, h, 28, 11, '.');
            stamp(g, w, h, 12, 27, '.');
            stamp(g, w, h, 20, 27, '.');
            stamp(g, w, h, 28, 27, '.');
            fillRect(g, w, h, 19, 14, 21, 25, '+');
            break;
        default:
            break;
    }
    placeObjectives();
    auto carve = [&](int x, int y) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int nx = x + dx;
                const int ny = y + dy;
                if (nx <= 0 || ny <= 0 || nx >= w - 1 || ny >= h - 1) continue;
                const char c = g[static_cast<size_t>(ny * w + nx)];
                if (c == '#' || c == '+' || c == 'X') {
                    g[static_cast<size_t>(ny * w + nx)] = '.';
                }
            }
        }
    };
    carve(4, 4);
    carve(5, 5);
    carve(w - 6, h - 6);
    carve(6, 6);
    carve(w - 7, h - 7);
    carve(12, 20);
    carve(20, 12);
    carve(28, 28);
    carve(20, 20);
    return loadFromGrid(id, mapName(id), themeFor(id), w, h, g);
}

} // namespace sbs
