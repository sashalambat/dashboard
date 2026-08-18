#pragma once

#include "Engine/SBSTypes.h"

namespace sbs {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float ix, float iy) : x(ix), y(iy) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }

    float lengthSq() const { return x * x + y * y; }
    float length() const { return std::sqrt(lengthSq()); }

    Vec2 normalized() const {
        const float l = length();
        if (l <= 0.0001f) {
            return {0.0f, 0.0f};
        }
        return {x / l, y / l};
    }
};

inline float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
inline float cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }
inline float distance(const Vec2& a, const Vec2& b) { return (a - b).length(); }
inline float distanceSq(const Vec2& a, const Vec2& b) { return (a - b).lengthSq(); }

inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

inline int clampi(int v, int lo, int hi) {
    return std::max(lo, std::min(hi, v));
}

inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float wrapAngle(float a) {
    while (a > 3.14159265f) a -= 6.2831853f;
    while (a < -3.14159265f) a += 6.2831853f;
    return a;
}

inline float angleTo(const Vec2& from, const Vec2& to) {
    const Vec2 d = to - from;
    return std::atan2(d.y, d.x);
}

inline Vec2 dirFromYaw(float yaw) {
    return {std::cos(yaw), std::sin(yaw)};
}

inline float angleDiff(float a, float b) {
    return wrapAngle(b - a);
}

struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    static Color rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return {r, g, b, a};
    }
};

inline Color factionColor(Faction f) {
    if (f == Faction::CyberDominion) {
        return Color::rgb(40, 220, 255);
    }
    return Color::rgb(255, 150, 40);
}

inline Color teamColor(TeamId t) {
    if (t == TeamId::Dominion) return Color::rgb(40, 220, 255);
    if (t == TeamId::Alliance) return Color::rgb(255, 150, 40);
    if (t == TeamId::Spectator) return Color::rgb(180, 180, 180);
    return Color::rgb(220, 220, 220);
}

class Rng {
public:
    explicit Rng(uint32_t seed = 1u) : state_(seed ? seed : 1u) {}

    uint32_t nextU32() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return state_;
    }

    float nextFloat() {
        return static_cast<float>(nextU32() & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
    }

    float range(float lo, float hi) {
        return lo + nextFloat() * (hi - lo);
    }

    int rangeInt(int lo, int hiInclusive) {
        if (hiInclusive <= lo) return lo;
        return lo + static_cast<int>(nextU32() % static_cast<uint32_t>(hiInclusive - lo + 1));
    }

private:
    uint32_t state_;
};

} // namespace sbs
