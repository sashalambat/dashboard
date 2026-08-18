#include "Game/UISystem.h"

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

} // namespace

int UISystem::menuCount(UiScreen screen) const {
    switch (screen) {
        case UiScreen::MainMenu: return 8;
        case UiScreen::Options: return 5;
        case UiScreen::Settings: return 6;
        case UiScreen::LanBrowser: return 6;
        case UiScreen::ClassSelect: return 5;
        case UiScreen::SpawnMenu: return 4;
        case UiScreen::Statistics: return 1;
        case UiScreen::Results: return 2;
        default: return 1;
    }
}

void UISystem::draw(Renderer& r, UiScreen screen, const SaveData& save, const World* world, uint8_t localId,
                    const std::vector<LanEntry>& servers, int cursor, const std::string& status) const {
    const Color title = Color::rgb(255, 160, 50);
    if (screen == UiScreen::MainMenu) {
        panel(r, 40, 20, 400, 230);
        r.drawText(60, 30, "SBS WARS", title, 3);
        r.drawText(60, 58, "LAN-ONLY SCI-FI FPS", Color::rgb(80, 200, 255), 1);
        const char* items[] = {
            "Host Listen Server", "Host Dedicated Server", "LAN Browser",
            "Offline vs Bots", "Options", "Statistics", "Settings", "Quit"
        };
        for (int i = 0; i < 8; ++i) item(r, 70, 80 + i * 14, items[i], cursor == i);
        r.drawText(60, 230, status, Color::rgb(160, 160, 160), 1);
        return;
    }
    if (screen == UiScreen::Options || screen == UiScreen::Settings) {
        panel(r, 40, 20, 400, 220);
        r.drawText(60, 30, screen == UiScreen::Options ? "OPTIONS" : "SETTINGS", title, 2);
        std::ostringstream ss;
        ss << "Name: " << save.playerName;
        item(r, 70, 70, ss.str(), cursor == 0);
        item(r, 70, 86, std::string("Quality: ") + (save.graphics.quality ? "High 120+" : "Low 60"), cursor == 1);
        item(r, 70, 102, std::string("Resolution: ") + std::to_string(save.graphics.width) + "x" + std::to_string(save.graphics.height), cursor == 2);
        item(r, 70, 118, std::string("Sensitivity: ") + std::to_string(save.input.mouseSensitivity), cursor == 3);
        item(r, 70, 134, std::string("Master Volume: ") + std::to_string(save.audio.master), cursor == 4);
        item(r, 70, 150, "Back", cursor == 5);
        return;
    }
    if (screen == UiScreen::LanBrowser) {
        panel(r, 20, 16, 440, 240);
        r.drawText(36, 24, "LAN SERVER BROWSER", title, 2);
        r.drawText(36, 48, "Refresh / Join / Favorite / Back", Color::rgb(180, 180, 180), 1);
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
        panel(r, 40, 20, 400, 230);
        r.drawText(60, 30, "MATCH RESULTS", title, 2);
        r.drawText(60, 60, world->lastAnnouncement(), Color::rgb(255, 220, 120), 1);
        r.drawText(60, 80, "Alliance " + std::to_string(world->scoreA()) + "   Dominion " + std::to_string(world->scoreB()),
                   Color::rgb(230, 230, 230), 1);
        int y = 100;
        for (const auto& p : world->players()) {
            if (!p.active) continue;
            std::ostringstream ss;
            ss << p.name << "  K:" << p.kills << " D:" << p.deaths << " S:" << p.score;
            r.drawText(70, y, ss.str(), factionColor(p.faction), 1);
            y += 10;
            if (y > 210) break;
        }
        item(r, 70, 220, "Continue", true);
        return;
    }

    if (!world) return;
    const PlayerState* me = world->playerById(localId);

    if (screen == UiScreen::Playing || screen == UiScreen::Scoreboard || screen == UiScreen::Spectator) {
        r.drawText(8, r.height() > 0 ? 1 : 1, std::string(modeName(world->mode())) + "  " +
                   std::to_string(world->scoreA()) + " - " + std::to_string(world->scoreB()) + "  " +
                   std::to_string(static_cast<int>(world->timeLeft())) + "s",
                   Color::rgb(240, 240, 240), 1);
        r.drawText(8, 12, world->lastAnnouncement(), Color::rgb(255, 200, 80), 1);
        if (me) {
            const WeaponRuntime& w = me->weapons[me->weaponSlot];
            r.drawText(8, 250, std::string(className(me->cls)) + "  " + weaponName(w.id) + "  " +
                       std::to_string(w.ammoInMag) + "/" + std::to_string(w.reserve),
                       Color::rgb(240, 240, 200), 1);
            r.drawRect(8, 262, std::max(1, static_cast<int>(me->health)), 6, Color::rgb(80, 220, 80));
            r.drawRect(8, 270, std::max(1, static_cast<int>(me->armor)), 4, Color::rgb(80, 160, 255));
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
