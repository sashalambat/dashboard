#include "Game/ClientApp.h"

#include "Engine/AudioEngine.h"
#include "Engine/NetSocket.h"
#include "Engine/Protocol.h"
#include "Engine/Renderer.h"
#include "Engine/SBSLog.h"
#include "Game/ServerHost.h"
#include "Game/UISystem.h"

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace sbs {
namespace {

UiScreen screen_ = UiScreen::MainMenu;
int cursor_ = 0;
bool running_ = true;
bool inMatch_ = false;
std::string status_;
std::vector<LanEntry> servers_;
UdpChannel browser_;
ServerHost host_;
World remoteWorld_;
GameMap remoteMap_;
bool usingRemote_ = false;
uint8_t localId_ = 0;
SocketAddr serverAddr_{};
UdpChannel clientNet_;
float discoverAcc_ = 0;
SaveData save_;
MatchConfig pending_;

void applyClassCursor() {
    save_.preferredClass = static_cast<PlayerClass>(clampi(cursor_, 0, 4));
}

void startLocalMatch(bool dedicatedLike, bool offline) {
    pending_.mode = GameModeId::TeamDeathmatch;
    pending_.map = MapId::SBSFoundry;
    pending_.botCount = 8;
    pending_.maxPlayers = 16;
    pending_.fillWithBots = true;
    pending_.difficulty = BotDifficulty::Veteran;
    pending_.sessionName = "SBS Wars LAN";
    pending_.listenServer = !dedicatedLike;
    pending_.dedicated = dedicatedLike && !offline;
    if (!host_.start(pending_, kLanPort, dedicatedLike && !offline)) {
        status_ = "Failed to start server (port in use?)";
        return;
    }
    usingRemote_ = false;
    const int id = host_.world().addPlayer(save_.playerName, save_.preferredClass, save_.preferredFaction, false, BotDifficulty::Veteran);
    if (id >= 0) localId_ = static_cast<uint8_t>(id);
    inMatch_ = true;
    screen_ = UiScreen::Playing;
    status_ = "Match live";
}

void sendDiscover() {
    uint8_t buf[32];
    const int n = packDiscoverQuery(buf, sizeof(buf));
    if (n > 0) browser_.sendBroadcast(kDiscoveryPort, buf, n);
    if (n > 0) browser_.sendBroadcast(kLanPort, buf, n);
}

void pollBrowser(float dt) {
    for (auto& s : servers_) s.age += dt;
    servers_.erase(std::remove_if(servers_.begin(), servers_.end(), [](const LanEntry& e) { return e.age > 6.0f; }), servers_.end());
    Packet pkt;
    while (browser_.recv(pkt)) {
        PacketType t{};
        if (!parsePacketType(pkt.bytes.data(), static_cast<int>(pkt.bytes.size()), t)) continue;
        if (t != PacketType::DiscoverReply) continue;
        ServerInfo info{};
        if (!parseDiscoverReply(pkt.bytes.data(), static_cast<int>(pkt.bytes.size()), info)) continue;
        bool found = false;
        for (auto& e : servers_) {
            if (e.addr.ipv4 == pkt.from.ipv4 && e.info.port == info.port) {
                e.info = info;
                e.age = 0;
                found = true;
                break;
            }
        }
        if (!found) {
            LanEntry e;
            e.info = info;
            e.addr = pkt.from;
            e.addr.port = info.port;
            servers_.push_back(e);
        }
    }
}

bool joinServer(const LanEntry& e) {
    if (!clientNet_.open(0, true)) return false;
    uint8_t buf[128];
    const int n = packJoinRequest(buf, sizeof(buf), save_.playerName.c_str(), save_.preferredClass, save_.preferredFaction);
    serverAddr_ = e.addr;
    serverAddr_.port = e.info.port;
    clientNet_.send(serverAddr_, buf, n);
    usingRemote_ = true;
    inMatch_ = true;
    screen_ = UiScreen::Playing;
    remoteMap_.loadBuiltin(static_cast<MapId>(0));
    MatchConfig cfg;
    cfg.mode = static_cast<GameModeId>(e.info.mode);
    remoteWorld_.reset(cfg, remoteMap_);
    status_ = "Joining...";
    return true;
}

void pollClientNet() {
    if (!usingRemote_ || !clientNet_.valid()) return;
    Packet pkt;
    while (clientNet_.recv(pkt)) {
        PacketType t{};
        if (!parsePacketType(pkt.bytes.data(), static_cast<int>(pkt.bytes.size()), t)) continue;
        if (t == PacketType::JoinAccept) {
            MapId map{};
            GameModeId mode{};
            if (parseJoinAccept(pkt.bytes.data(), static_cast<int>(pkt.bytes.size()), localId_, map, mode)) {
                remoteMap_.loadBuiltin(map);
                MatchConfig cfg;
                cfg.mode = mode;
                cfg.map = map;
                remoteWorld_.reset(cfg, remoteMap_);
                status_ = "Joined LAN match";
            }
        } else if (t == PacketType::Snapshot) {
            std::vector<NetPlayerState> ps;
            std::vector<NetProjectile> pr;
            std::vector<NetObjective> ob;
            uint32_t tick = 0;
            MatchState st{};
            GameModeId mode{};
            float tl = 0;
            int16_t a = 0, b = 0;
            if (parseSnapshot(pkt.bytes.data(), static_cast<int>(pkt.bytes.size()), tick, st, mode, tl, a, b, ps, pr, ob)) {
                (void)tick;
                (void)st;
                (void)mode;
                (void)tl;
                (void)a;
                (void)b;
                (void)pr;
                (void)ob;
                for (const auto& np : ps) {
                    PlayerState* p = remoteWorld_.playerById(np.id);
                    if (!p) {
                        remoteWorld_.addPlayer(np.name, static_cast<PlayerClass>(np.classAndTeam & 7),
                                              static_cast<Faction>(np.faction), (np.flags & 2) != 0, BotDifficulty::Veteran);
                        p = remoteWorld_.playerById(np.id);
                    }
                    if (!p) continue;
                    p->pos.x = np.x / 32.0f;
                    p->pos.y = np.y / 32.0f;
                    p->alive = (np.flags & 1) != 0;
                    p->kills = np.kills;
                    p->deaths = np.deaths;
                    p->score = np.score;
                    p->health = np.hp;
                    p->yaw = (np.yaw / 255.0f) * 6.2831853f - 3.14159265f;
                }
            }
        }
    }
}

World& activeWorld() {
    return usingRemote_ ? remoteWorld_ : host_.world();
}

void stopMatch() {
    inMatch_ = false;
    usingRemote_ = false;
    host_.stop();
    clientNet_.close();
    screen_ = UiScreen::MainMenu;
}

} // namespace

int runDedicatedServer(int argc, char** argv) {
    const LaunchOptions o = parseLaunchOptions(argc, argv);
    MatchConfig cfg;
    cfg.mode = GameModeId::TeamDeathmatch;
    cfg.map = MapId::SBSFoundry;
    cfg.botCount = o.bots;
    cfg.maxPlayers = o.maxPlayers;
    cfg.sessionName = o.session;
    cfg.fillWithBots = true;
    cfg.dedicated = true;
    ServerHost server;
    if (!server.start(cfg, static_cast<uint16_t>(o.port), true)) return 1;
    logInfo("SBS Wars dedicated server running. Ctrl+C to stop.");
    const float dt = kTickDt;
    auto last = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        float real = std::chrono::duration<float>(now - last).count();
        last = now;
        float acc = std::min(real, 0.1f);
        while (acc >= dt) {
            server.update(dt);
            acc -= dt;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int runClient(int argc, char** argv) {
    LaunchOptions o = parseLaunchOptions(argc, argv);
    save_ = SaveSystem::load();
    if (!o.name.empty() && o.name != "Operator") save_.playerName = o.name;

    if (o.dedicated) {
        return runDedicatedServer(argc, argv);
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
        logError(std::string("SDL_Init failed: ") + SDL_GetError());
        if (o.offline || o.listen) {
            logWarn("Falling back to headless listen simulation");
            startLocalMatch(false, true);
            for (int i = 0; i < 600; ++i) host_.update(kTickDt);
            return host_.world().aliveCount() > 0 ? 0 : 1;
        }
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(kGameTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          save_.graphics.width, save_.graphics.height,
                                          SDL_WINDOW_SHOWN | (save_.graphics.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
    if (!window) {
        logError("Failed to create window");
        SDL_Quit();
        return 1;
    }

    Renderer renderer;
    if (!renderer.init(window, save_.graphics)) {
        logError("Renderer init failed");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    AudioEngine audio;
    audio.init(save_.audio);
    UISystem ui;
    browser_.open(0, true);
    sendDiscover();

    if (o.offline || o.listen) {
        startLocalMatch(false, o.offline);
        audio.playAnnouncement("Match start");
    }

    SDL_SetRelativeMouseMode(inMatch_ ? SDL_TRUE : SDL_FALSE);
    auto last = std::chrono::steady_clock::now();
    float inputYaw = 0.0f;
    bool tab = false;
    std::string lastAnn;

    while (running_) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last).count();
        last = now;
        dt = clampf(dt, 0.0f, 0.1f);

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running_ = false;
            if (e.type == SDL_KEYDOWN) {
                const SDL_Keycode k = e.key.keysym.sym;
                if (!inMatch_) {
                    if (k == SDLK_ESCAPE) {
                        if (screen_ == UiScreen::MainMenu) running_ = false;
                        else screen_ = UiScreen::MainMenu;
                    }
                    if (k == SDLK_UP) cursor_ = std::max(0, cursor_ - 1);
                    if (k == SDLK_DOWN) cursor_ = cursor_ + 1;
                    if (k == SDLK_TAB && screen_ == UiScreen::ClassSelect) {
                        save_.preferredFaction = save_.preferredFaction == Faction::SBSAlliance ? Faction::CyberDominion : Faction::SBSAlliance;
                    }
                    if (k == SDLK_r && screen_ == UiScreen::LanBrowser) sendDiscover();
                    if (k == SDLK_f && screen_ == UiScreen::LanBrowser && cursor_ < static_cast<int>(servers_.size())) {
                        SaveSystem::addFavorite(save_, servers_[static_cast<size_t>(cursor_)].addr.toString());
                        SaveSystem::store(save_);
                        status_ = "Favorite saved";
                    }
                    if (k == SDLK_RETURN) {
                        audio.playUi("select");
                        if (screen_ == UiScreen::MainMenu) {
                            if (cursor_ == 0) startLocalMatch(false, false);
                            else if (cursor_ == 1) startLocalMatch(true, false);
                            else if (cursor_ == 2) {
                                screen_ = UiScreen::LanBrowser;
                                sendDiscover();
                                cursor_ = 0;
                            } else if (cursor_ == 3) startLocalMatch(false, true);
                            else if (cursor_ == 4) {
                                screen_ = UiScreen::Options;
                                cursor_ = 0;
                            } else if (cursor_ == 5) {
                                screen_ = UiScreen::Statistics;
                                cursor_ = 0;
                            } else if (cursor_ == 6) {
                                screen_ = UiScreen::Settings;
                                cursor_ = 0;
                            } else if (cursor_ == 7) running_ = false;
                        } else if (screen_ == UiScreen::ClassSelect) {
                            applyClassCursor();
                            screen_ = UiScreen::SpawnMenu;
                            cursor_ = 0;
                        } else if (screen_ == UiScreen::SpawnMenu) {
                            if (cursor_ == 0) screen_ = UiScreen::Playing;
                            if (cursor_ == 1) screen_ = UiScreen::ClassSelect;
                            if (cursor_ == 2) {
                                PlayerInput in{};
                                in.requestSpectate = true;
                                activeWorld().setInput(localId_, in);
                                screen_ = UiScreen::Spectator;
                            }
                            if (cursor_ == 3) stopMatch();
                        } else if (screen_ == UiScreen::LanBrowser) {
                            if (cursor_ < static_cast<int>(servers_.size())) joinServer(servers_[static_cast<size_t>(cursor_)]);
                        } else if (screen_ == UiScreen::Options || screen_ == UiScreen::Settings || screen_ == UiScreen::Statistics || screen_ == UiScreen::Results) {
                            screen_ = UiScreen::MainMenu;
                            cursor_ = 0;
                        }
                    }
                } else {
                    if (k == SDLK_ESCAPE) {
                        screen_ = UiScreen::SpawnMenu;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                    }
                    if (k == SDLK_TAB) tab = true;
                    if (k == SDLK_F1) {
                        PlayerState* me = activeWorld().playerById(localId_);
                        if (me) me->upgradeLevel = clampi(me->upgradeLevel + 1, 0, 5);
                    }
                }
            }
            if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_TAB) tab = false;
            if (e.type == SDL_MOUSEMOTION && inMatch_ && screen_ == UiScreen::Playing) {
                inputYaw += e.motion.xrel * save_.input.mouseSensitivity * 0.01f;
            }
        }

        discoverAcc_ += dt;
        if (discoverAcc_ > 2.0f) {
            discoverAcc_ = 0;
            if (screen_ == UiScreen::LanBrowser) sendDiscover();
        }
        pollBrowser(dt);
        pollClientNet();

        if (inMatch_) {
            if (!usingRemote_) host_.update(dt);
            PlayerState* me = activeWorld().playerById(localId_);
            if (me && screen_ == UiScreen::Playing) {
                const Uint8* keys = SDL_GetKeyboardState(nullptr);
                PlayerInput in = me->input;
                in.moveY = (keys[SDL_SCANCODE_W] ? 1.0f : 0.0f) + (keys[SDL_SCANCODE_S] ? -1.0f : 0.0f);
                in.moveX = (keys[SDL_SCANCODE_D] ? 1.0f : 0.0f) + (keys[SDL_SCANCODE_A] ? -1.0f : 0.0f);
                // convert WASD from view space
                const Vec2 f = dirFromYaw(inputYaw);
                const Vec2 r{-f.y, f.x};
                const Vec2 wish = f * in.moveY + r * in.moveX;
                in.moveX = wish.x;
                in.moveY = wish.y;
                in.yaw = inputYaw;
                in.fire = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
                in.altFire = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
                in.reload = keys[SDL_SCANCODE_R] != 0;
                in.sprint = keys[SDL_SCANCODE_LSHIFT] != 0;
                in.weaponSlot = keys[SDL_SCANCODE_2] ? 1 : 0;
                activeWorld().setInput(localId_, in);
                if (usingRemote_ && clientNet_.valid()) {
                    uint8_t buf[128];
                    const int n = packClientInput(buf, sizeof(buf), localId_, activeWorld().tickIndex(), in);
                    clientNet_.send(serverAddr_, buf, n);
                }
                audio.updateListener(me->pos, me->yaw);
                if (me->firing) audio.playWeapon(me->weapons[me->weaponSlot].id, me->faction, 0.0f);
            }
            if (activeWorld().lastAnnouncement() != lastAnn) {
                lastAnn = activeWorld().lastAnnouncement();
                audio.playAnnouncement(lastAnn.c_str());
                audio.playVoice(save_.preferredFaction, lastAnn.c_str());
            }
            if (activeWorld().matchOver()) {
                screen_ = UiScreen::Results;
                save_.stats.matchesPlayed += 1;
                if (me) {
                    save_.stats.kills += me->kills;
                    save_.stats.deaths += me->deaths;
                    save_.stats.captures += me->captures;
                }
                SaveSystem::store(save_);
                inMatch_ = false;
                SDL_SetRelativeMouseMode(SDL_FALSE);
            }
            if (tab) screen_ = UiScreen::Scoreboard;
            else if (screen_ == UiScreen::Scoreboard) screen_ = UiScreen::Playing;
        }

        World* drawWorld = inMatch_ || screen_ == UiScreen::Results ? &activeWorld() : nullptr;
        if (drawWorld) {
            const bool spec = screen_ == UiScreen::Spectator || (drawWorld->playerById(localId_) && drawWorld->playerById(localId_)->spectating);
            renderer.renderWorld(*drawWorld, localId_, spec);
        } else {
            // clear via empty fill
            World dummy;
            GameMap map;
            map.loadBuiltin(MapId::CyberCore);
            dummy.reset(MatchConfig{}, map);
            renderer.renderWorld(dummy, 0, true);
        }
        ui.draw(renderer, screen_, save_, drawWorld, localId_, servers_, cursor_, status_);
        renderer.present();
    }

    SaveSystem::store(save_);
    audio.shutdown();
    renderer.shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace sbs
