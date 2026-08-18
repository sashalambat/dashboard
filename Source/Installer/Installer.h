#pragma once

#include <string>

namespace sbs {

struct InstallOptions {
    std::string payload;
    std::string prefix;
    bool desktopShortcut = true;
    bool menuShortcut = true;
    bool launchAfter = false;
    bool gui = true;
};

int runInstaller(int argc, char** argv);

} // namespace sbs
