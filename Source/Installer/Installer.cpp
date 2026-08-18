#include "Installer/Installer.h"

#include "Engine/Renderer.h"
#include "Engine/SBSLog.h"
#include "Game/SaveSystem.h"

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace sbs {
namespace {

std::string homeDir() {
#if defined(_WIN32)
    const char* p = std::getenv("USERPROFILE");
    if (p && p[0]) return p;
    const char* h = std::getenv("HOME");
    return h ? h : ".";
#else
    const char* p = std::getenv("HOME");
    if (p && p[0]) return p;
    if (passwd* pw = getpwuid(getuid())) {
        if (pw->pw_dir) return pw->pw_dir;
    }
    return ".";
#endif
}

std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (a.back() == '/' || a.back() == '\\') return a + b;
    return a + "/" + b;
}

std::string dirnameOf(const std::string& p) {
    const auto slash = p.find_last_of("/\\");
    if (slash == std::string::npos) return ".";
    if (slash == 0) return "/";
    return p.substr(0, slash);
}

[[maybe_unused]] std::string shellQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

int runInstallScript(const InstallOptions& opt) {
#if defined(_WIN32)
    std::string script = joinPath(opt.payload, "install.ps1");
    if (script.find("install.ps1") != std::string::npos) {
        std::ifstream in(script);
        if (!in) {
            script = joinPath(joinPath(opt.payload, "Packaging"), "windows");
            script = joinPath(script, "install.ps1");
        } else {
            in.close();
        }
    }
    std::ostringstream cmd;
    cmd << "powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" << script
        << "\" -Prefix \"" << opt.prefix << "\" -Payload \"" << opt.payload << "\"";
    if (!opt.desktopShortcut) cmd << " -NoDesktop";
    if (!opt.menuShortcut) cmd << " -NoStartMenu";
    logInfo("Installing to " + opt.prefix);
    const int rc = std::system(cmd.str().c_str());
    if (rc != 0) {
        logError("Windows install script failed");
        return 1;
    }
    return 0;
#else
    std::string script = joinPath(opt.payload, "install.sh");
    std::ifstream in(script);
    if (!in) {
        logError("Missing install.sh in payload: " + script);
        return 1;
    }
    in.close();
    std::ostringstream cmd;
    cmd << "CREATE_DESKTOP=" << (opt.desktopShortcut ? "1" : "0")
        << " CREATE_MENU=" << (opt.menuShortcut ? "1" : "0")
        << " bash " << shellQuote(script)
        << " --payload " << shellQuote(opt.payload)
        << " --prefix " << shellQuote(opt.prefix);
    logInfo("Installing to " + opt.prefix);
    const int rc = std::system(cmd.str().c_str());
    if (rc != 0) {
        logError("Install script failed");
        return 1;
    }
    return 0;
#endif
}

InstallOptions parseInstallArgs(int argc, char** argv) {
    InstallOptions o;
    o.prefix = joinPath(homeDir(), "SBSWars");
#if defined(_WIN32)
    if (const char* app = std::getenv("LOCALAPPDATA")) {
        o.prefix = joinPath(app, "SBSWars");
    }
#endif
    const char* self = (argc > 0 && argv[0]) ? argv[0] : ".";
    std::string selfDir = dirnameOf(self);
    o.payload = dirnameOf(selfDir);
    if (o.payload == "." || o.payload.empty()) o.payload = selfDir;
    // Prefer payload next to the installer binary (extracted layout: payload/bin/sbswars-installer)
    if (selfDir.size() >= 4 && selfDir.substr(selfDir.size() - 4) == "/bin") {
        o.payload = dirnameOf(selfDir);
    }
    o.gui = std::getenv("DISPLAY") != nullptr || std::getenv("WAYLAND_DISPLAY") != nullptr;
#if defined(_WIN32)
    o.gui = true;
#endif
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i] ? argv[i] : "";
        auto next = [&](std::string& dst) {
            if (i + 1 < argc && argv[i + 1]) dst = argv[++i];
        };
        if (a == "--prefix") next(o.prefix);
        else if (a == "--payload") next(o.payload);
        else if (a == "--cli" || a == "--headless") o.gui = false;
        else if (a == "--no-desktop") o.desktopShortcut = false;
        else if (a == "--no-menu") o.menuShortcut = false;
        else if (a == "--launch") o.launchAfter = true;
    }
    return o;
}

void drawToggle(Renderer& r, int x, int y, const std::string& label, bool on, bool selected) {
    const Color box = on ? Color::rgb(80, 220, 80) : Color::rgb(80, 80, 80);
    r.drawRect(x, y, 12, 12, box);
    r.drawText(x + 18, y + 2, label, selected ? Color::rgb(255, 200, 80) : Color::rgb(220, 220, 220), 1);
}

} // namespace

int runInstaller(int argc, char** argv) {
    InstallOptions opt = parseInstallArgs(argc, argv);
    if (!opt.gui) {
        const int rc = runInstallScript(opt);
        if (rc == 0) {
            logInfo("Desktop shortcut created (unless disabled).");
        }
        return rc;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        logWarn("GUI unavailable, installing from command line");
        return runInstallScript(opt);
    }
    SDL_Window* window = SDL_CreateWindow("SBS Wars Setup", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 840, 520, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Quit();
        return runInstallScript(opt);
    }
    GraphicsSettings gfx;
    gfx.width = 840;
    gfx.height = 520;
    gfx.vsync = true;
    gfx.quality = 1;
    Renderer renderer;
    if (!renderer.init(window, gfx)) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return runInstallScript(opt);
    }

    int cursor = 0;
    bool installed = false;
    std::string status = "Choose options, then Install";
    bool running = true;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type != SDL_KEYDOWN) continue;
            const SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_ESCAPE) running = false;
            if (!installed) {
                if (k == SDLK_UP) cursor = std::max(0, cursor - 1);
                if (k == SDLK_DOWN) cursor = std::min(3, cursor + 1);
                if (k == SDLK_SPACE || k == SDLK_LEFT || k == SDLK_RIGHT) {
                    if (cursor == 1) opt.desktopShortcut = !opt.desktopShortcut;
                    if (cursor == 2) opt.menuShortcut = !opt.menuShortcut;
                }
                if (k == SDLK_RETURN) {
                    if (cursor == 3 || cursor == 0) {
                        status = "Installing...";
                        renderer.drawRect(0, 0, 480, 270, Color::rgb(10, 12, 16));
                        renderer.drawTextCenter(120, "INSTALLING...", Color::rgb(255, 160, 50), 2);
                        renderer.present();
                        if (runInstallScript(opt) == 0) {
                            installed = true;
                            status = "Installed. Desktop shortcut is on your Desktop.";
                            cursor = 0;
                        } else {
                            status = "Install failed. See console log.";
                        }
                    }
                }
            } else {
                if (k == SDLK_RETURN) {
                    if (cursor == 0) {
#if defined(_WIN32)
                        const std::string launch = joinPath(opt.prefix, "SBSWarsLauncher.exe");
                        const int launched = std::system(("start \"\" \"" + launch + "\"").c_str());
#else
                        const std::string launch = joinPath(opt.prefix, "SBSWarsLauncher");
                        const int launched = std::system((shellQuote(launch) + " >/dev/null 2>&1 &").c_str());
#endif
                        if (launched != 0) {
                            logWarn("Could not start launcher automatically");
                        }
                    }
                    running = false;
                }
                if (k == SDLK_UP || k == SDLK_DOWN) cursor = cursor == 0 ? 1 : 0;
            }
        }

        renderer.drawRect(0, 0, 480, 270, Color::rgb(10, 12, 18));
        renderer.drawRect(0, 0, 480, 8, Color::rgb(255, 150, 40));
        renderer.drawRect(0, 262, 480, 8, Color::rgb(40, 200, 255));
        renderer.drawText(24, 18, "SBS WARS SETUP", Color::rgb(255, 160, 50), 2);
        renderer.drawText(24, 42, "LAN-only sci-fi military FPS", Color::rgb(80, 200, 255), 1);

        if (!installed) {
            renderer.drawText(24, 70, "Install to:", Color::rgb(180, 180, 180), 1);
            renderer.drawText(24, 84, opt.prefix, cursor == 0 ? Color::rgb(255, 220, 120) : Color::rgb(230, 230, 230), 1);
            drawToggle(renderer, 24, 112, "Create desktop shortcut", opt.desktopShortcut, cursor == 1);
#if defined(_WIN32)
            drawToggle(renderer, 24, 132, "Create Start Menu shortcut", opt.menuShortcut, cursor == 2);
#else
            drawToggle(renderer, 24, 132, "Add to applications menu", opt.menuShortcut, cursor == 2);
#endif
            renderer.drawRect(24, 168, 160, 22, cursor == 3 ? Color::rgb(255, 150, 40) : Color::rgb(40, 50, 60));
            renderer.drawText(48, 174, "INSTALL", Color::rgb(20, 20, 20), 1);
            renderer.drawText(24, 210, "Arrows move  Space toggles  Enter installs", Color::rgb(140, 140, 140), 1);
        } else {
            renderer.drawText(24, 80, "Setup complete.", Color::rgb(80, 220, 120), 1);
            renderer.drawText(24, 98, status, Color::rgb(230, 230, 230), 1);
#if defined(_WIN32)
            renderer.drawText(24, 120, "Desktop\\SBS Wars.lnk", Color::rgb(255, 200, 80), 1);
#else
            renderer.drawText(24, 120, joinPath(homeDir(), "Desktop") + "/SBS Wars.desktop", Color::rgb(255, 200, 80), 1);
#endif
            renderer.drawRect(24, 160, 140, 22, cursor == 0 ? Color::rgb(255, 150, 40) : Color::rgb(40, 50, 60));
            renderer.drawText(40, 166, "LAUNCH", Color::rgb(20, 20, 20), 1);
            renderer.drawRect(180, 160, 140, 22, cursor == 1 ? Color::rgb(255, 150, 40) : Color::rgb(40, 50, 60));
            renderer.drawText(210, 166, "CLOSE", Color::rgb(20, 20, 20), 1);
        }
        renderer.drawText(24, 244, status, Color::rgb(180, 180, 180), 1);
        renderer.present();
        SDL_Delay(16);
    }

    renderer.shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace sbs

int main(int argc, char** argv) {
    sbs::logInfo(std::string("SBS Wars Installer ") + SBS_VERSION);
    return sbs::runInstaller(argc, argv);
}
