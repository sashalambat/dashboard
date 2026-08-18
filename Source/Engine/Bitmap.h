#pragma once

#include "Engine/SBSMath.h"

#include <string>
#include <vector>

namespace sbs {

struct Bitmap {
    int width = 0;
    int height = 0;
    std::vector<Color> pixels;

    bool empty() const { return width <= 0 || height <= 0 || pixels.empty(); }
    bool loadPng(const std::string& path);
    Color sample(float u, float v) const;
    Color pixel(int x, int y) const;
};

} // namespace sbs
