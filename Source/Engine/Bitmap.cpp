#include "Engine/Bitmap.h"

#include "Engine/AssetPath.h"

#include <cmath>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace sbs {

bool Bitmap::loadPng(const std::string& path) {
    const std::string resolved = findAsset(path);
    int w = 0, h = 0, n = 0;
    unsigned char* data = stbi_load(resolved.c_str(), &w, &h, &n, 4);
    if (!data || w <= 0 || h <= 0) {
        if (data) stbi_image_free(data);
        width = 0;
        height = 0;
        pixels.clear();
        return false;
    }
    width = w;
    height = h;
    pixels.resize(static_cast<size_t>(w * h));
    for (int i = 0; i < w * h; ++i) {
        Color c;
        c.r = data[i * 4 + 0];
        c.g = data[i * 4 + 1];
        c.b = data[i * 4 + 2];
        c.a = data[i * 4 + 3];
        pixels[static_cast<size_t>(i)] = c;
    }
    stbi_image_free(data);
    return true;
}

Color Bitmap::pixel(int x, int y) const {
    if (empty()) return {};
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= width) x = width - 1;
    if (y >= height) y = height - 1;
    return pixels[static_cast<size_t>(y * width + x)];
}

Color Bitmap::sample(float u, float v) const {
    if (empty()) return {};
    u -= std::floor(u);
    v -= std::floor(v);
    if (u < 0) u += 1.0f;
    v = clampf(v, 0.0f, 0.9999f);
    const int x = static_cast<int>(u * static_cast<float>(width)) % width;
    const int y = static_cast<int>(v * static_cast<float>(height)) % height;
    return pixel((x + width) % width, (y + height) % height);
}

} // namespace sbs
