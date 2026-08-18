#pragma once

#include <string>
#include <vector>

namespace sbs {

void initAssetSearch(const char* argv0);
void addAssetSearchDir(const std::string& dir);
std::string findAsset(const std::string& relative);
std::vector<std::string> assetSearchDirs();

} // namespace sbs
