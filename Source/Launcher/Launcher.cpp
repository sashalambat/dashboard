#include "Engine/SBSLog.h"
#include "Engine/BitmapFont.h"

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {

std::string exeDir(const char* argv0) {
    std::string p = argv0 ? argv0 : ".";
    const auto slash = p.find_last_of("/\\");
    if (slash == std::string::npos) return ".";
    return p.substr(0, slash);
}

bool fileExists(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fclose(f);
    return true;
}

std::string firstExisting(const std::vector<std::string>& paths) {
    for (const auto& p : paths) {
        if (fileExists(p)) return p;
    }
    return paths.empty() ? std::string() : paths.front();
}

int spawn(const std::string& path, const std::vector<std::string>& args) {
    std::vector<char*> cargs;
    cargs.push_back(const_cast<char*>(path.c_str()));
    std::vector<std::string> storage = args;
    for (auto& a : storage) cargs.push_back(const_cast<char*>(a.c_str()));
    cargs.push_back(nullptr);
#if defined(_WIN32)
    return _spawnv(_P_NOWAIT, path.c_str(), cargs.data()) == -1 ? 1 : 0;
#else
    pid_t pid = fork();
    if (pid == 0) {
        execv(path.c_str(), cargs.data());
        _exit(127);
    }
    return pid < 0 ? 1 : 0;
#endif
}

void activate(int cursor, const std::string& client, const std::string& server, bool& run) {
    if (cursor == 0) spawn(client, {});
    if (cursor == 1) spawn(server, {"--dedicated", "--bots", "12"});
    if (cursor == 2) spawn(client, {"--offline"});
    if (cursor == 3) run = false;
}

} // namespace

int main(int argc, char** argv) {
    sbs::logInfo(std::string("SBS Wars Launcher ") + SBS_VERSION);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        sbs::logError("Launcher SDL init failed");
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("SBS Wars Launcher", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 420, 0);
    if (!window) {
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* r = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!r) {
        r = SDL_CreateRenderer(window, -1, 0);
    }
    if (!r) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const std::string dir = exeDir(argc > 0 ? argv[0] : ".");
#if defined(_WIN32)
    const std::string client = firstExisting({dir + "\\SBSWars.exe", dir + "\\sbswars.exe"});
    const std::string server = firstExisting({dir + "\\SBSWarsServer.exe", dir + "\\sbswars-server.exe"});
#else
    const std::string client = firstExisting({dir + "/sbswars", dir + "/SBSWars"});
    const std::string server = firstExisting({dir + "/sbswars-server", dir + "/SBSWarsServer"});
#endif
    int cursor = 0;
    bool run = true;
    const char* items[] = {"Launch Game", "Host Dedicated Server", "Offline vs Bots", "Quit"};
    const SDL_Rect buttons[4] = {
        {80, 120, 560, 48},
        {80, 180, 560, 48},
        {80, 240, 560, 48},
        {80, 300, 560, 48},
    };

    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) run = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) run = false;
                if (e.key.keysym.sym == SDLK_UP) cursor = std::max(0, cursor - 1);
                if (e.key.keysym.sym == SDLK_DOWN) cursor = std::min(3, cursor + 1);
                if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) {
                    activate(cursor, client, server, run);
                }
            }
            if (e.type == SDL_MOUSEMOTION) {
                const int mx = e.motion.x;
                const int my = e.motion.y;
                for (int i = 0; i < 4; ++i) {
                    if (mx >= buttons[i].x && mx < buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my < buttons[i].y + buttons[i].h) {
                        cursor = i;
                    }
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                const int mx = e.button.x;
                const int my = e.button.y;
                for (int i = 0; i < 4; ++i) {
                    if (mx >= buttons[i].x && mx < buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my < buttons[i].y + buttons[i].h) {
                        cursor = i;
                        activate(cursor, client, server, run);
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(r, 10, 12, 16, 255);
        SDL_RenderClear(r);
        SDL_SetRenderDrawColor(r, 255, 150, 40, 255);
        SDL_Rect top{0, 0, 720, 10};
        SDL_RenderFillRect(r, &top);
        SDL_SetRenderDrawColor(r, 40, 200, 255, 255);
        SDL_Rect bot{0, 410, 720, 10};
        SDL_RenderFillRect(r, &bot);

        sbs::drawSdlText(r, 80, 28, "SBS WARS", SDL_Color{255, 160, 50, 255}, 5);
        sbs::drawSdlText(r, 80, 78, "LAN-ONLY SCI-FI FPS", SDL_Color{80, 200, 255, 255}, 2);

        for (int i = 0; i < 4; ++i) {
            if (i == cursor) SDL_SetRenderDrawColor(r, 255, 160, 50, 255);
            else SDL_SetRenderDrawColor(r, 40, 50, 60, 255);
            SDL_RenderFillRect(r, &buttons[i]);
            const SDL_Color label = (i == cursor) ? SDL_Color{20, 16, 10, 255} : SDL_Color{230, 230, 230, 255};
            const int scale = 3;
            const int tw = sbs::sdlTextWidth(items[i], scale);
            const int tx = buttons[i].x + (buttons[i].w - tw) / 2;
            const int ty = buttons[i].y + (buttons[i].h - 7 * scale) / 2;
            sbs::drawSdlText(r, tx, ty, items[i], label, scale);
        }
        sbs::drawSdlText(r, 80, 368, "ARROWS OR MOUSE   ENTER TO START", SDL_Color{160, 160, 160, 255}, 2);
        SDL_RenderPresent(r);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
