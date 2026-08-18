#include "Game/ClientApp.h"

#include "Engine/SBSLog.h"
#include "Game/ServerHost.h"

#include <chrono>
#include <thread>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <csignal>
#endif

namespace {
volatile bool gRun = true;
#if defined(_WIN32)
BOOL WINAPI consoleHandler(DWORD) {
    gRun = false;
    return TRUE;
}
#else
void onStop(int) { gRun = false; }
#endif
} // namespace

int main(int argc, char** argv) {
    sbs::logInfo(std::string("SBS Wars ") + SBS_VERSION + " dedicated server");
#if defined(_WIN32)
    SetConsoleCtrlHandler(consoleHandler, TRUE);
#else
    std::signal(SIGINT, onStop);
    std::signal(SIGTERM, onStop);
#endif
    sbs::LaunchOptions o = sbs::parseLaunchOptions(argc, argv);
    sbs::MatchConfig cfg;
    cfg.mode = sbs::GameModeId::TeamDeathmatch;
    cfg.map = sbs::MapId::SBSFoundry;
    cfg.botCount = o.bots;
    cfg.maxPlayers = o.maxPlayers;
    cfg.sessionName = o.session.empty() ? "SBS Wars Dedicated" : o.session;
    cfg.fillWithBots = true;
    cfg.dedicated = true;
    sbs::ServerHost server;
    if (!server.start(cfg, static_cast<uint16_t>(o.port), true)) {
        return 1;
    }
    auto last = std::chrono::steady_clock::now();
    while (gRun) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last).count();
        last = now;
        dt = sbs::clampf(dt, 0.0f, 0.1f);
        float acc = dt;
        while (acc >= sbs::kTickDt) {
            server.update(sbs::kTickDt);
            acc -= sbs::kTickDt;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    server.stop();
    return 0;
}
