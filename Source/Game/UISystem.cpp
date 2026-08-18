#include "Game/UISystem.h"

#include "Game/InputMap.h"

#include <algorithm>
#include <sstream>

namespace sbs {
namespace {

void panel(Renderer& r, int x, int y, int w, int h) {
    r.drawRect(x, y, w, h, Color::rgb(8, 10, 14, 220));
    r.drawRect(x, y, w, 2, Color::rgb(255, 150, 40));
    r.drawRect(x, y + h - 2, w, 2, Color::rgb(40, 200, 255));
}

void item(Renderer& r, int x, int y, const std::string& text, bool selected) {
    r.drawText(x, y, selected ? (std::string("> ") + text) : (std::string("  ") + text),
               selected ? Color::rgb(255, 200, 80) : Color::rgb(210, 210, 210), 1);
}

bool inRect(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && mx < x + w && my >= y && my < y + h;
}

int rowHit(int mx, int my, int x, int y0, int rows, int rowH, int w) {
    for (int i = 0; i < rows; ++i) {
        if (inRect(mx, my, x, y0 + i * rowH, w, rowH)) return i;
    }
    return -1;
}

} // namespace

int UISystem::menuCount(UiScreen screen) const {
    switch (screen) {
        case UiScreen::MainMenu: return 8;
        case UiScreen::Options: return 6;
        case UiScreen::Settings: return static_cast<int>(BindSlot::Count) + 2;
        case UiScreen::LanBrowser: return 6;
        case UiScreen::ClassSelect: return 5;
        case UiScreen::SpawnMenu: return 4;
        case UiScreen::Statistics: return 1;
        case UiScreen::Results: return 2;
        default: return 1;
    }
}

int UISystem::hitTest(UiScreen screen, int mx, int my, int serverCount) const {
    switch (screen) {
        case UiScreen::MainMenu: return rowHit(mx, my, 50, 78, 8, 14, 360);
        case UiScreen::Options: return rowHit(mx, my, 50, 68, 6, 16, 360);
        case UiScreen::Settings: return rowHit(mx, my, 40, 48, menuCount(screen), 12, 400);
        case UiScreen::LanBrowser: {
            const int n = std::min(serverCount, 8);
            const int row = rowHit(mx, my, 20, 68, n, 14, 420);
            if (row >= 0) return row;
            return -1;
        }
        case UiScreen::ClassSelect: return rowHit(mx, my, 50, 68, 5, 16, 360);
        case UiScreen::SpawnMenu: return rowHit(mx, my, 50, 78, 4, 16, 360);
        case UiScreen::Statistics: return inRect(mx, my, 50, 176, 300, 16) ? 0 : -1;
        case UiScreen::Results: return rowHit(mx, my, 50, 188, 2, 14, 360);
        default: return -1;
    }
}

void UISystem::draw(Renderer& r, UiScreen screen, const SaveData& save, const World* world, uint8_t localId,
                    const std::vector<LanEntry>& servers, int cursor, const std::string& status,
                    int waitingBind) const {
    const Color title = Color::rgb(255, 160, 50);
    if (screen == UiScreen::MainMenu) {
        panel(r, 40, 20, 400, 230);
        r.drawText(60, 30, "SBS WARS", title, 3);
        r.drawText(60, 58, "LAN-ONLY SCI-FI FPS", Color::rgb(80, 200, 255), 1);
        const char* items[] = {
            "Host Listen Server", "Host Dedicated Server", "LAN Browser",
            "Offline vs Bots", "Options", "Statistics", "Controls", "Quit"
        };
        for (int i = 0; i < 8; ++i) item(r, 70, 80 + i * 14, items[i], cursor == i);
        r.drawText(60, 230, status, Color::rgb(160, 160, 160), 1);
        return;
    }
    if (screen == UiScreen::Options) {
        panel(r, 40, 20, 400, 220);
        r.drawText(60, 30, "OPTIONS", title, 2);
        std::ostringstream ss;
        ss << "Name: " << save.playerName;
        item(r, 70, 70, ss.str(), cursor == 0);
        item(r, 70, 86, std::string("Quality: ") + (save.graphics.quality ? "High" : "Low"), cursor == 1);
        item(r, 70, 102, std::string("Resolution: ") + std::to_string(save.graphics.width) + "x" + std::to_string(save.graphics.height), cursor == 2);
        item(r, 70, 118, std::string("Sensitivity: ") + std::to_string(save.input.mouseSensitivity), cursor == 3);
        item(r, 70, 134, std::string("Master Volume: ") + std::to_string(save.audio.master), cursor == 4);
        item(r, 70, 150, "Back", cursor == 5);
        r.drawText(60, 200, "Left/Right to change   Enter Back", Color::rgb(160, 160, 160), 1);
        return;
    }
    if (screen == UiScreen::Settings) {
        panel(r, 20, 8, 440, 254);
        r.drawText(36, 14, "CONTROLS", title, 2);
        const int bindCount = static_cast<int>(BindSlot::Count);
        for (int i = 0; i < bindCount; ++i) {
            const BindSlot slot = static_cast<BindSlot>(i);
            std::string line = std::string(bindLabel(slot)) + "   ";
            if (waitingBind == i) line += "[PRESS KEY OR MOUSE]";
            else line += bindName(bindValue(save.input, slot));
            item(r, 40, 50 + i * 12, line, cursor == i);
        }
        item(r, 40, 50 + bindCount * 12, std::string("Sensitivity: ") + std::to_string(save.input.mouseSensitivity), cursor == bindCount);
        item(r, 40, 50 + (bindCount + 1) * 12, "Back", cursor == bindCount + 1);
        r.drawText(36, 246, "Click a bind then press a key", Color::rgb(160, 160, 160), 1);
        return;
    }
    if (screen == UiScreen::LanBrowser) {
        panel(r, 20, 16, 440, 240);
        r.drawText(36, 24, "LAN SERVER BROWSER", title, 2);
        r.drawText(36, 48, "Click a server or Enter to join", Color::rgb(180, 180, 180), 1);
        if (servers.empty()) {
            r.drawText(36, 80, "Scanning LAN... no servers yet", Color::rgb(200, 200, 80), 1);
        }
        for (size_t i = 0; i < servers.size() && i < 8; ++i) {
            const auto& s = servers[i];
            std::ostringstream line;
            line << s.info.sessionName << " | " << s.info.mapName << " | "
                 << modeName(static_cast<GameModeId>(s.info.mode)) << " | "
                 << static_cast<int>(s.info.players) << "/" << static_cast<int>(s.info.maxPlayers)
                 << (s.info.dedicated ? " DED" : " LISTEN");
            item(r, 36, 70 + static_cast<int>(i) * 14, line.str(), cursor == static_cast<int>(i));
        }
        item(r, 36, 220, status.empty() ? "Enter=Join  R=Refresh  F=Favorite  Esc=Back" : status, false);
        return;
    }
    if (screen == UiScreen::ClassSelect) {
        panel(r, 40, 20, 400, 220);
        r.drawText(60, 30, "SELECT CLASS", title, 2);
        for (int i = 0; i < 5; ++i) {
            const PlayerClass c = static_cast<PlayerClass>(i);
            const ClassDef& d = classDef(c);
            std::ostringstream ss;
            ss << className(c) << "  HP " << static_cast<int>(d.maxHealth) << "  "
               << weaponName(d.primary);
            item(r, 70, 70 + i * 16, ss.str(), cursor == i);
        }
        r.drawText(60, 200, "Faction: " + std::string(factionName(save.preferredFaction)), Color::rgb(80, 200, 255), 1);
        return;
    }
    if (screen == UiScreen::SpawnMenu) {
        panel(r, 40, 30, 400, 180);
        r.drawText(60, 40, "SPAWN MENU", title, 2);
        item(r, 70, 80, "Deploy to battlefield", cursor == 0);
        item(r, 70, 96, "Change class", cursor == 1);
        item(r, 70, 112, "Spectate", cursor == 2);
        item(r, 70, 128, "Disconnect", cursor == 3);
        return;
    }
    if (screen == UiScreen::Statistics) {
        panel(r, 40, 20, 400, 220);
        r.drawText(60, 30, "STATISTICS", title, 2);
        r.drawText(70, 70, "Matches: " + std::to_string(save.stats.matchesPlayed), Color::rgb(230, 230, 230), 1);
        r.drawText(70, 86, "Wins: " + std::to_string(save.stats.wins), Color::rgb(230, 230, 230), 1);
        r.drawText(70, 102, "Kills: " + std::to_string(save.stats.kills), Color::rgb(230, 230, 230), 1);
        r.drawText(70, 118, "Deaths: " + std::to_string(save.stats.deaths), Color::rgb(230, 230, 230), 1);
        r.drawText(70, 134, "Captures: " + std::to_string(save.stats.captures), Color::rgb(230, 230, 230), 1);
        item(r, 70, 180, "Back", true);
        return;
    }
    if (screen == UiScreen::Results && world) {
        panel(r, 40, 8, 400, 210);
        r.drawText(60, 16, "MATCH RESULTS", title, 2);
        r.drawText(60, 40, world->lastAnnouncement(), Color::rgb(255, 220, 120), 1);
        r.drawText(60, 56, "Alliance " + std::to_string(world->scoreA()) + "   Dominion " + std::to_string(world->scoreB()),
                   Color::rgb(230, 230, 230), 1);
        int y = 72;
        for (const auto& p : world->players()) {
            if (!p.active) continue;
            std::ostringstream ss;
            ss << p.name << "  K:" << p.kills << " D:" << p.deaths << " S:" << p.score;
            r.drawText(70, y, ss.str(), factionColor(p.faction), 1);
            y += 10;
            if (y > 176) break;
        }
        item(r, 70, 188, "Continue", cursor == 0);
        item(r, 70, 202, "Play Again", cursor == 1);
        return;
    }

    if (!world) return;
    const PlayerState* me = world->playerById(localId);
    const int fh = r.frameHeight();

    if (screen == UiScreen::Playing || screen == UiScreen::Scoreboard || screen == UiScreen::Spectator) {
        r.drawText(8, 1, std::string(modeName(world->mode())) + "  " +
                   std::to_string(world->scoreA()) + " - " + std::to_string(world->scoreB()) + "  " +
                   std::to_string(static_cast<int>(world->timeLeft())) + "s",
                   Color::rgb(240, 240, 240), 1);
        r.drawText(8, 12, world->lastAnnouncement(), Color::rgb(255, 200, 80), 1);
        if (world->state() == MatchState::Warmup) {
            r.drawText(8, 24, "WARMUP - weapons live when the match starts", Color::rgb(80, 220, 255), 1);
        }
        if (me) {
            const WeaponRuntime& w = me->weapons[me->weaponSlot];
            r.drawText(8, fh - 28, std::string(className(me->cls)) + "  " + weaponName(w.id) + "  " +
                       std::to_string(w.ammoInMag) + "/" + std::to_string(w.reserve),
                       Color::rgb(240, 240, 200), 1);
            r.drawRect(8, fh - 16, std::max(1, static_cast<int>(me->health)), 6, Color::rgb(80, 220, 80));
            r.drawRect(8, fh - 9, std::max(1, static_cast<int>(me->armor)), 4, Color::rgb(80, 160, 255));
            r.drawText(200, fh - 16, "1/2 guns  LMB fire  walk over pickups", Color::rgb(170, 170, 170), 1);
        }
        if (screen == UiScreen::Spectator) {
            r.drawTextCenter(20, "SPECTATOR MODE", Color::rgb(200, 200, 200), 2);
        }
    }
    if (screen == UiScreen::Scoreboard && world) {
        panel(r, 30, 30, 420, 220);
        r.drawText(50, 40, "SCOREBOARD", title, 2);
        int y = 70;
        for (const auto& p : world->players()) {
            if (!p.active) continue;
            std::ostringstream ss;
            ss << (p.isBot ? "[AI] " : "") << p.name << "  " << className(p.cls)
               << "  K " << p.kills << "  D " << p.deaths << "  " << p.score;
            r.drawText(50, y, ss.str(), teamColor(p.team), 1);
            y += 11;
            if (y > 230) break;
        }
    }
}

} // namespace sbs
