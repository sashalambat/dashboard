#include "Engine/AssetPath.h"

#include <cstdio>

#if defined(_WIN32)
#include <cstring>
#endif

namespace sbs {
namespace {

std::vector<std::string> gRoots = {
    "Content/art",
    "Content",
    ".",
};

std::string dirnameOf(const std::string& p) {
    const auto slash = p.find_last_of("/\\");
    if (slash == std::string::npos) return ".";
    return p.substr(0, slash);
}

bool existsFile(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fclose(f);
    return true;
}

std::string join(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (a.back() == '/' || a.back() == '\\') return a + b;
    return a + "/" + b;
}

} // namespace

void addAssetSearchDir(const std::string& dir) {
    if (dir.empty()) return;
    for (const auto& r : gRoots) {
        if (r == dir) return;
    }
    gRoots.insert(gRoots.begin(), dir);
}

void initAssetSearch(const char* argv0) {
    addAssetSearchDir("Content/art");
    addAssetSearchDir("Content");
    if (argv0 && argv0[0]) {
        const std::string d = dirnameOf(argv0);
        addAssetSearchDir(join(d, "Content/art"));
        addAssetSearchDir(join(d, "Content"));
        addAssetSearchDir(join(join(d, ".."), "Content/art"));
    }
}

std::string findAsset(const std::string& relative) {
    const std::string cands[] = {
        relative,
        std::string("art/") + relative,
        std::string("Content/art/") + relative,
        std::string("Content/") + relative,
    };
    for (const auto& root : gRoots) {
        for (const auto& c : cands) {
            const std::string path = join(root, c);
            if (existsFile(path)) return path;
            if (existsFile(c)) return c;
        }
    }
    return relative;
}

std::vector<std::string> assetSearchDirs() { return gRoots; }

} // namespace sbs
