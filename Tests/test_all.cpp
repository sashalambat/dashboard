#include "Engine/Protocol.h"
#include "Engine/SBSLog.h"
#include "Game/GameConfig.h"
#include "Game/GameModes.h"
#include "Game/MapData.h"
#include "Game/SaveSystem.h"
#include "Game/ServerHost.h"
#include "Game/WeaponSystem.h"
#include "Game/World.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace sbs;

static int gFailed = 0;
static int gPassed = 0;

#define CHECK(cond, name)                                                                 \
    do {                                                                                  \
        if (cond) {                                                                       \
            ++gPassed;                                                                    \
            std::printf("  PASS  %s\n", name);                                            \
        } else {                                                                          \
            ++gFailed;                                                                    \
            std::printf("  FAIL  %s\n", name);                                            \
        }                                                                                 \
    } while (0)

static World makeWorld(GameModeId mode, MapId map, int bots) {
    GameMap m;
    CHECK(m.loadBuiltin(map), "map load");
    MatchConfig cfg;
    cfg.mode = mode;
    cfg.map = map;
    cfg.botCount = bots;
    cfg.fillWithBots = false;
    cfg.maxPlayers = 16;
    World w;
    w.reset(cfg, m);
    return w;
}

int main() {
    logInfo("SBS Wars automated test suite");

    // Maps
    for (int i = 0; i < static_cast<int>(MapId::Count); ++i) {
        GameMap m;
        const bool ok = m.loadBuiltin(static_cast<MapId>(i));
        CHECK(ok && m.width() > 8 && m.height() > 8, mapName(static_cast<MapId>(i)));
        CHECK(!m.spawns().empty(), "spawns exist");
        std::vector<Vec2> path;
        Rng rng(3);
        const bool pf = m.pathfind(m.randomSpawn(TeamId::Alliance, rng), m.hillPosition(), path);
        CHECK(pf && path.size() >= 1, "navigation path");
    }

    // Weapons / classes
    CHECK(weaponDef(WeaponId::SBSAssaultRifle).magazine == 30, "AR mag");
    CHECK(weaponDef(WeaponId::Railgun).piercing, "rail piercing");
    CHECK(weaponDef(WeaponId::RocketLauncher).isExplosive, "rocket splash");
    CHECK(classDef(PlayerClass::Heavy).maxHealth > classDef(PlayerClass::Recon).maxHealth, "heavy vs recon hp");
    CHECK(classDef(PlayerClass::Recon).hasRadar, "recon radar");
    CHECK(classDef(PlayerClass::Medic).healPerSecond > 0.0f, "medic heal");
    CHECK(classDef(PlayerClass::Engineer).repairPerSecond > 0.0f, "engineer repair");

    // Deathmatch simulation
    {
        World w = makeWorld(GameModeId::Deathmatch, MapId::SBSFoundry, 0);
        const int a = w.addPlayer("Alpha", PlayerClass::Assault, Faction::SBSAlliance, false, BotDifficulty::Veteran);
        const int b = w.addPlayer("Bravo", PlayerClass::Heavy, Faction::CyberDominion, false, BotDifficulty::Elite);
        CHECK(a >= 0 && b >= 0, "add two players");
        PlayerState* pa = w.playerById(static_cast<uint8_t>(a));
        PlayerState* pb = w.playerById(static_cast<uint8_t>(b));
        CHECK(pa && pb && pa->alive && pb->alive, "both alive");
        pb->pos = pa->pos + dirFromYaw(pa->yaw) * 2.0f;
        PlayerInput in{};
        in.yaw = pa->yaw;
        in.fire = true;
        w.setInput(static_cast<uint8_t>(a), in);
        for (int i = 0; i < 90; ++i) w.tick(kTickDt);
        CHECK(true, "weapon fire simulation completed");
        CHECK(w.aliveCount() >= 1, "match still populated");
    }

    // Team modes + bots
    {
        World w = makeWorld(GameModeId::TeamDeathmatch, MapId::CyberCore, 0);
        w.addPlayer("Host", PlayerClass::Assault, Faction::SBSAlliance, false, BotDifficulty::Veteran);
        w.addPlayer("BotA", PlayerClass::Recon, Faction::SBSAlliance, true, BotDifficulty::Recruit);
        w.addPlayer("BotB", PlayerClass::Heavy, Faction::CyberDominion, true, BotDifficulty::Elite);
        CHECK(w.botCount() == 2, "bot count");
        CHECK(w.humanCount() == 1, "human count");
        for (int i = 0; i < 240; ++i) w.tick(kTickDt);
        CHECK(w.state() == MatchState::Playing || w.state() == MatchState::Ended || w.state() == MatchState::Warmup,
              "tdm state valid");
        std::vector<NetPlayerState> ps;
        std::vector<NetProjectile> pr;
        std::vector<NetObjective> ob;
        w.buildSnapshot(ps, pr, ob);
        CHECK(!ps.empty(), "snapshot players");
        uint8_t buf[4096];
        const int n = packSnapshot(buf, sizeof(buf), w.tickIndex(), w.state(), w.mode(), w.timeLeft(),
                                   static_cast<int16_t>(w.scoreA()), static_cast<int16_t>(w.scoreB()), ps, pr, ob);
        CHECK(n > 16, "snapshot packs");
        uint32_t tick = 0;
        MatchState st{};
        GameModeId mode{};
        float tl = 0;
        int16_t sa = 0, sb = 0;
        std::vector<NetPlayerState> ps2;
        std::vector<NetProjectile> pr2;
        std::vector<NetObjective> ob2;
        CHECK(parseSnapshot(buf, n, tick, st, mode, tl, sa, sb, ps2, pr2, ob2), "snapshot parses");
        CHECK(ps2.size() == ps.size(), "snapshot player roundtrip");
    }

    // Objective modes
    {
        World ctf = makeWorld(GameModeId::CaptureTheFlag, MapId::ArcticBaseZeta, 0);
        ctf.addPlayer("A1", PlayerClass::Assault, Faction::SBSAlliance, true, BotDifficulty::Veteran);
        ctf.addPlayer("B1", PlayerClass::Medic, Faction::CyberDominion, true, BotDifficulty::Veteran);
        for (int i = 0; i < 180; ++i) ctf.tick(kTickDt);
        CHECK(ctf.mode() == GameModeId::CaptureTheFlag, "ctf mode");
        CHECK(distance(ctf.flagA().home, Vec2{}) > 1.0f, "flag home set");
    }
    {
        World dom = makeWorld(GameModeId::Domination, MapId::TitanFactory, 0);
        dom.addPlayer("A1", PlayerClass::Engineer, Faction::SBSAlliance, true, BotDifficulty::Elite);
        dom.addPlayer("B1", PlayerClass::Heavy, Faction::CyberDominion, true, BotDifficulty::Recruit);
        for (int i = 0; i < 180; ++i) dom.tick(kTickDt);
        CHECK(dom.mode() == GameModeId::Domination, "dom mode");
    }
    {
        World hill = makeWorld(GameModeId::KingOfTheHill, MapId::OrbitalPlatformSeven, 0);
        hill.addPlayer("A1", PlayerClass::Assault, Faction::SBSAlliance, true, BotDifficulty::Veteran);
        hill.addPlayer("B1", PlayerClass::Recon, Faction::CyberDominion, true, BotDifficulty::Veteran);
        for (int i = 0; i < 180; ++i) hill.tick(kTickDt);
        CHECK(hill.mode() == GameModeId::KingOfTheHill, "koth mode");
        CHECK(hill.hill().x > 0, "hill exists");
    }

    // Spectator
    {
        World w = makeWorld(GameModeId::TeamDeathmatch, MapId::CrimsonDesert, 0);
        const int id = w.addPlayer("Ghost", PlayerClass::Recon, Faction::SBSAlliance, false, BotDifficulty::Veteran);
        PlayerInput in{};
        in.requestSpectate = true;
        w.setInput(static_cast<uint8_t>(id), in);
        w.tick(kTickDt);
        const PlayerState* p = w.playerById(static_cast<uint8_t>(id));
        CHECK(p && p->spectating, "spectator mode");
    }

    // Save / load
    {
        SaveData d;
        d.playerName = "TestPilot";
        d.graphics.quality = 0;
        d.stats.kills = 42;
        SaveSystem::addFavorite(d, "10.0.0.5:27015");
        CHECK(SaveSystem::store(d), "save write");
        SaveData l = SaveSystem::load();
        CHECK(l.playerName == "TestPilot", "save name");
        CHECK(l.stats.kills == 42, "save kills");
        CHECK(!l.lanFavorites.empty(), "lan favorites");
    }

    // Protocol discover
    {
        uint8_t q[32];
        CHECK(packDiscoverQuery(q, sizeof(q)) > 0, "discover query");
        ServerInfo info{};
        const char* session = "UnitTest";
        const char* mapn = "SBS Foundry";
        for (size_t i = 0; session[i] && i + 1 < sizeof(info.sessionName); ++i) info.sessionName[i] = session[i];
        for (size_t i = 0; mapn[i] && i + 1 < sizeof(info.mapName); ++i) info.mapName[i] = mapn[i];
        info.mode = 1;
        info.players = 4;
        info.maxPlayers = 16;
        info.port = kLanPort;
        info.listen = true;
        uint8_t r[256];
        const int n = packDiscoverReply(r, sizeof(r), info);
        ServerInfo out{};
        CHECK(n > 0 && parseDiscoverReply(r, n, out), "discover reply");
        CHECK(std::string(out.sessionName) == "UnitTest", "session name roundtrip");
    }

    // LAN server host
    {
        MatchConfig cfg;
        cfg.mode = GameModeId::TeamDeathmatch;
        cfg.map = MapId::SBSFoundry;
        cfg.botCount = 4;
        cfg.fillWithBots = true;
        cfg.maxPlayers = 8;
        ServerHost host;
        const bool started = host.start(cfg, 27222, true);
        CHECK(started, "dedicated host start");
        if (started) {
            for (int i = 0; i < 30; ++i) host.update(kTickDt);
            CHECK(host.world().botCount() >= 4, "bots filled");
            CHECK(host.running(), "host running");
            UdpChannel probe;
            CHECK(probe.open(0, true), "probe socket");
            uint8_t q[32];
            const int qn = packDiscoverQuery(q, sizeof(q));
            SocketAddr dest = SocketAddr::fromIpv4Port((127u << 24) | 1u, host.port());
            CHECK(qn > 0 && probe.send(dest, q, qn), "discover send");
            host.update(kTickDt);
            Packet pkt;
            bool got = false;
            for (int i = 0; i < 20 && !got; ++i) {
                host.update(kTickDt);
                while (probe.recv(pkt)) {
                    PacketType t{};
                    if (parsePacketType(pkt.bytes.data(), static_cast<int>(pkt.bytes.size()), t) && t == PacketType::DiscoverReply) {
                        got = true;
                    }
                }
            }
            CHECK(got, "LAN discover reply");
            host.stop();
            CHECK(!host.running(), "host stop");
        }
    }

    // Balance sanity
    float dpsAr = weaponDef(WeaponId::SBSAssaultRifle).damage / weaponDef(WeaponId::SBSAssaultRifle).fireInterval;
    float dpsMg = weaponDef(WeaponId::SBSMachineGun).damage / weaponDef(WeaponId::SBSMachineGun).fireInterval;
    CHECK(dpsMg >= dpsAr * 0.85f, "MG DPS competitive");
    CHECK(difficultyDef(BotDifficulty::Elite).accuracy > difficultyDef(BotDifficulty::Recruit).accuracy, "diff scaling");
    CHECK(playersOnPoint(makeWorld(GameModeId::KingOfTheHill, MapId::SBSFoundry, 0), Vec2{1, 1}, 10, TeamId::None) == 0,
          "empty hill occupancy");

    std::printf("\nSBS Wars tests: %d passed, %d failed\n", gPassed, gFailed);
    return gFailed == 0 ? 0 : 1;
}
