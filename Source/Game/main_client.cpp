#include "Game/ClientApp.h"

#include "Engine/SBSLog.h"

int main(int argc, char** argv) {
    sbs::logInfo(std::string("SBS Wars ") + SBS_VERSION + " client");
    return sbs::runClient(argc, argv);
}
