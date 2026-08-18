#pragma once

#include "Game/SaveSystem.h"

#include <string>

namespace sbs {

int runClient(int argc, char** argv);
int runDedicatedServer(int argc, char** argv);

struct LaunchOptions {
    bool dedicated = false;
    bool listen = false;
    bool offline = false;
    bool join = false;
    bool headless = false;
    std::string connect;
    std::string name = "Operator";
    std::string session = "SBS Wars LAN";
    int bots = 8;
    int maxPlayers = 16;
    int port = 27015;
};

LaunchOptions parseLaunchOptions(int argc, char** argv);

} // namespace sbs
