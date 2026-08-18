#include "Game/ClientApp.h"

#include <cstdlib>
#include <string>

namespace sbs {

LaunchOptions parseLaunchOptions(int argc, char** argv) {
    LaunchOptions o;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i] ? argv[i] : "";
        auto next = [&](std::string& dst) {
            if (i + 1 < argc && argv[i + 1]) dst = argv[++i];
        };
        if (a == "--dedicated" || a == "-d") o.dedicated = true;
        else if (a == "--listen") o.listen = true;
        else if (a == "--offline") o.offline = true;
        else if (a == "--headless") o.headless = true;
        else if (a == "--join" && i + 1 < argc) {
            o.join = true;
            next(o.connect);
        } else if (a == "--name") next(o.name);
        else if (a == "--session") next(o.session);
        else if (a == "--bots" && i + 1 < argc) o.bots = std::atoi(argv[++i]);
        else if (a == "--max" && i + 1 < argc) o.maxPlayers = std::atoi(argv[++i]);
        else if (a == "--port" && i + 1 < argc) o.port = std::atoi(argv[++i]);
    }
    return o;
}

} // namespace sbs
