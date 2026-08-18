#include "Game/ClientApp.h"

#include "Engine/AudioEngine.h"
#include "Engine/NetSocket.h"
#include "Engine/Protocol.h"
#include "Engine/Renderer.h"
#include "Engine/SBSLog.h"
#include "Game/InputMap.h"
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
int waitingBind_ = -1;
int weaponSlot_ = 0;
bool lastOffline_ = true;
bool resultsRecorded_ = false;

void applyClassCursor() {
    save_.preferredClass = static_cast<PlayerClass>(clampi(cursor_, 0, 4));
}

void startLocalMatch(bool dedicatedLike, bool offline) {
    lastOffline_ = offline;
    resultsRecorded_ = false;
    pending_.mode = GameModeId::TeamDeathmatch;
    pending_.map = MapId::SBSFoundry;
    pending_.botCount = 6;
    pending_.maxPlayers = 16;
    pending_.fillWithBots = true;
    pending_.difficulty = BotDifficulty::Recruit;
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
    weaponSlot_ = 0;
    status_ = "Match live";
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_ShowCursor(SDL_DISABLE);
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
    cursor_ = 0;
    waitingBind_ = -1;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_ENABLE);
}

bool isGameplayScreen() {
    return screen_ == UiScreen::Playing || screen_ == UiScreen::Scoreboard || screen_ == UiScreen::Spectator;
}

void setCursorVisible(bool show) {
    SDL_SetRelativeMouseMode(show ? SDL_FALSE : SDL_TRUE);
    SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

void nudgeOption(int dir) {
    if (screen_ == UiScreen::Options) {
        if (cursor_ == 1) save_.graphics.quality = save_.graphics.quality ? 0 : 1;
        if (cursor_ == 2) {
            if (dir > 0) {
                save_.graphics.width = save_.graphics.width >= 1920 ? 1280 : 1920;
                save_.graphics.height = save_.graphics.width == 1920 ? 1080 : 720;
            } else {
                save_.graphics.width = save_.graphics.width <= 1280 ? 1920 : 1280;
                save_.graphics.height = save_.graphics.width == 1920 ? 1080 : 720;
            }
        }
        if (cursor_ == 3) save_.input.mouseSensitivity = clampf(save_.input.mouseSensitivity + dir * 0.04f, 0.04f, 1.5f);
        if (cursor_ == 4) save_.audio.master = clampf(save_.audio.master + dir * 0.1f, 0.0f, 1.0f);
    }
    if (screen_ == UiScreen::Settings && cursor_ == static_cast<int>(BindSlot::Count)) {
        save_.input.mouseSensitivity = clampf(save_.input.mouseSensitivity + dir * 0.04f, 0.04f, 1.5f);
    }
}

void activateMenu(AudioEngine& audio) {
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
        if (cursor_ == 0) {
            screen_ = UiScreen::Playing;
            setCursorVisible(false);
        }
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
    } else if (screen_ == UiScreen::Options) {
        if (cursor_ == 5) {
            SaveSystem::store(save_);
            screen_ = UiScreen::MainMenu;
            cursor_ = 0;
        } else {
            nudgeOption(1);
        }
    } else if (screen_ == UiScreen::Settings) {
        const int bindCount = static_cast<int>(BindSlot::Count);
        if (cursor_ == bindCount + 1) {
            SaveSystem::store(save_);
            screen_ = UiScreen::MainMenu;
            cursor_ = 0;
        } else if (cursor_ == bindCount) {
            nudgeOption(1);
        } else if (cursor_ >= 0 && cursor_ < bindCount) {
            waitingBind_ = cursor_;
            status_ = "Press a key or mouse button...";
        }
    } else if (screen_ == UiScreen::Statistics) {
        screen_ = UiScreen::MainMenu;
        cursor_ = 0;
    } else if (screen_ == UiScreen::Results) {
        if (cursor_ <= 0) {
            stopMatch();
        } else {
            stopMatch();
            startLocalMatch(false, lastOffline_);
        }
    }
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
    SDL_ShowCursor(inMatch_ ? SDL_DISABLE : SDL_ENABLE);
    auto last = std::chrono::steady_clock::now();
    float inputYaw = 0.0f;
    std::string lastAnn;

    auto mapMouse = [&](int mx, int my, int& fx, int& fy) {
        int ww = 1, wh = 1;
        SDL_GetWindowSize(window, &ww, &wh);
        fx = mx * renderer.frameWidth() / std::max(1, ww);
        fy = my * renderer.frameHeight() / std::max(1, wh);
    };

    while (running_) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last).count();
        last = now;
        dt = clampf(dt, 0.0f, 0.1f);

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running_ = false;

            if (waitingBind_ >= 0) {
                if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        waitingBind_ = -1;
                        status_.clear();
                    } else {
                        bindRef(save_.input, static_cast<BindSlot>(waitingBind_)) = static_cast<int>(e.key.keysym.scancode);
                        waitingBind_ = -1;
                        status_ = "Bind saved";
                        SaveSystem::store(save_);
                    }
                } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                    bindRef(save_.input, static_cast<BindSlot>(waitingBind_)) = encodeMouseBind(e.button.button);
                    waitingBind_ = -1;
                    status_ = "Bind saved";
                    SaveSystem::store(save_);
                }
                continue;
            }

            const bool menu = !isGameplayScreen();
            if (e.type == SDL_KEYDOWN) {
                const SDL_Keycode k = e.key.keysym.sym;
                if (menu) {
                    if (k == SDLK_ESCAPE) {
                        if (screen_ == UiScreen::MainMenu) running_ = false;
                        else if (screen_ == UiScreen::SpawnMenu) {
                            screen_ = UiScreen::Playing;
                            setCursorVisible(false);
                        } else {
                            screen_ = UiScreen::MainMenu;
                            cursor_ = 0;
                        }
                    }
                    if (k == SDLK_UP) cursor_ = std::max(0, cursor_ - 1);
                    if (k == SDLK_DOWN) cursor_ = std::min(ui.menuCount(screen_) - 1, cursor_ + 1);
                    if (k == SDLK_LEFT) nudgeOption(-1);
                    if (k == SDLK_RIGHT) nudgeOption(1);
                    if (k == SDLK_TAB && screen_ == UiScreen::ClassSelect) {
                        save_.preferredFaction = save_.preferredFaction == Faction::SBSAlliance ? Faction::CyberDominion : Faction::SBSAlliance;
                    }
                    if (k == SDLK_r && screen_ == UiScreen::LanBrowser) sendDiscover();
                    if (k == SDLK_f && screen_ == UiScreen::LanBrowser && cursor_ < static_cast<int>(servers_.size())) {
                        SaveSystem::addFavorite(save_, servers_[static_cast<size_t>(cursor_)].addr.toString());
                        SaveSystem::store(save_);
                        status_ = "Favorite saved";
                    }
                    if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) {
                        activateMenu(audio);
                    }
                } else {
                    if (k == SDLK_ESCAPE) {
                        screen_ = UiScreen::SpawnMenu;
                        setCursorVisible(true);
                        cursor_ = 0;
                    }
                    if (k == SDLK_F1) {
                        PlayerState* me = activeWorld().playerById(localId_);
                        if (me) me->upgradeLevel = clampi(me->upgradeLevel + 1, 0, 5);
                    }
                }
            }
            if (e.type == SDL_MOUSEMOTION) {
                if (inMatch_ && screen_ == UiScreen::Playing) {
                    inputYaw += e.motion.xrel * save_.input.mouseSensitivity * 0.08f;
                } else if (menu) {
                    int fx = 0, fy = 0;
                    mapMouse(e.motion.x, e.motion.y, fx, fy);
                    const int hit = ui.hitTest(screen_, fx, fy, static_cast<int>(servers_.size()));
                    if (hit >= 0) cursor_ = hit;
                }
            }
            if (e.type == SDL_MOUSEWHEEL && inMatch_ && screen_ == UiScreen::Playing) {
                weaponSlot_ = e.wheel.y > 0 ? 0 : 1;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && menu) {
                int fx = 0, fy = 0;
                mapMouse(e.button.x, e.button.y, fx, fy);
                const int hit = ui.hitTest(screen_, fx, fy, static_cast<int>(servers_.size()));
                if (hit >= 0) {
                    cursor_ = hit;
                    activateMenu(audio);
                } else if (screen_ == UiScreen::Results) {
                    cursor_ = 0;
                    activateMenu(audio);
                }
            }
        }

        cursor_ = clampi(cursor_, 0, std::max(0, ui.menuCount(screen_) - 1));

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
                const float forward = (bindHeld(keys, save_.input.moveForward) ? 1.0f : 0.0f) +
                                      (bindHeld(keys, save_.input.moveBack) ? -1.0f : 0.0f);
                const float strafe = (bindHeld(keys, save_.input.moveRight) ? 1.0f : 0.0f) +
                                     (bindHeld(keys, save_.input.moveLeft) ? -1.0f : 0.0f);
                const Vec2 f = dirFromYaw(inputYaw);
                const Vec2 right{-f.y, f.x};
                const Vec2 wish = f * forward + right * strafe;
                in.moveX = wish.x;
                in.moveY = wish.y;
                in.yaw = inputYaw;
                in.jump = bindHeld(keys, save_.input.jump);
                in.fire = bindHeld(keys, save_.input.fire);
                in.altFire = bindHeld(keys, save_.input.altFire);
                in.reload = bindHeld(keys, save_.input.reload);
                in.sprint = bindHeld(keys, save_.input.sprint);
                if (bindHeld(keys, save_.input.weapon1)) weaponSlot_ = 0;
                if (bindHeld(keys, save_.input.weapon2)) weaponSlot_ = 1;
                in.weaponSlot = static_cast<uint8_t>(clampi(weaponSlot_, 0, 1));
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
                cursor_ = 0;
                if (!resultsRecorded_) {
                    resultsRecorded_ = true;
                    save_.stats.matchesPlayed += 1;
                    if (me) {
                        save_.stats.kills += me->kills;
                        save_.stats.deaths += me->deaths;
                        save_.stats.captures += me->captures;
                        if (me->team == TeamId::Alliance && activeWorld().scoreA() > activeWorld().scoreB()) save_.stats.wins += 1;
                        if (me->team == TeamId::Dominion && activeWorld().scoreB() > activeWorld().scoreA()) save_.stats.wins += 1;
                    }
                    SaveSystem::store(save_);
                }
                inMatch_ = false;
                setCursorVisible(true);
            }
            if (screen_ == UiScreen::Playing || screen_ == UiScreen::Scoreboard) {
                const Uint8* keys = SDL_GetKeyboardState(nullptr);
                if (bindHeld(keys, save_.input.scoreboard)) screen_ = UiScreen::Scoreboard;
                else if (screen_ == UiScreen::Scoreboard) screen_ = UiScreen::Playing;
            }
        }

        World* drawWorld = inMatch_ || screen_ == UiScreen::Results ? &activeWorld() : nullptr;
        if (drawWorld) {
            const bool spec = screen_ == UiScreen::Spectator || (drawWorld->playerById(localId_) && drawWorld->playerById(localId_)->spectating);
            renderer.renderWorld(*drawWorld, localId_, spec);
        } else {
            World dummy;
            GameMap map;
            map.loadBuiltin(MapId::CyberCore);
            dummy.reset(MatchConfig{}, map);
            renderer.renderWorld(dummy, 0, true);
        }
        ui.draw(renderer, screen_, save_, drawWorld, localId_, servers_, cursor_, status_, waitingBind_);
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
