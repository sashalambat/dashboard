#pragma once

#include "Game/SaveSystem.h"

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include <string>

namespace sbs {

enum class BindSlot {
    Forward = 0,
    Back,
    Left,
    Right,
    Sprint,
    Reload,
    Jump,
    Weapon1,
    Weapon2,
    Scoreboard,
    Fire,
    AltFire,
    Count
};

inline int& bindRef(InputSettings& in, BindSlot slot) {
    switch (slot) {
        case BindSlot::Forward: return in.moveForward;
        case BindSlot::Back: return in.moveBack;
        case BindSlot::Left: return in.moveLeft;
        case BindSlot::Right: return in.moveRight;
        case BindSlot::Sprint: return in.sprint;
        case BindSlot::Reload: return in.reload;
        case BindSlot::Jump: return in.jump;
        case BindSlot::Weapon1: return in.weapon1;
        case BindSlot::Weapon2: return in.weapon2;
        case BindSlot::Scoreboard: return in.scoreboard;
        case BindSlot::Fire: return in.fire;
        case BindSlot::AltFire: return in.altFire;
        default: return in.moveForward;
    }
}

inline int bindValue(const InputSettings& in, BindSlot slot) {
    return bindRef(const_cast<InputSettings&>(in), slot);
}

inline const char* bindLabel(BindSlot slot) {
    switch (slot) {
        case BindSlot::Forward: return "Move Forward";
        case BindSlot::Back: return "Move Back";
        case BindSlot::Left: return "Move Left";
        case BindSlot::Right: return "Move Right";
        case BindSlot::Sprint: return "Sprint";
        case BindSlot::Reload: return "Reload";
        case BindSlot::Jump: return "Jump";
        case BindSlot::Weapon1: return "Primary Weapon";
        case BindSlot::Weapon2: return "Secondary Weapon";
        case BindSlot::Scoreboard: return "Scoreboard";
        case BindSlot::Fire: return "Fire";
        case BindSlot::AltFire: return "Alt Fire";
        default: return "Bind";
    }
}

inline bool bindIsMouse(int code) { return code < 0; }

inline std::string bindName(int code) {
    if (code == 0) return "Unbound";
    if (code == -static_cast<int>(SDL_BUTTON_LEFT)) return "Left Mouse";
    if (code == -static_cast<int>(SDL_BUTTON_RIGHT)) return "Right Mouse";
    if (code == -static_cast<int>(SDL_BUTTON_MIDDLE)) return "Middle Mouse";
    if (code < 0) return "Mouse Button";
    const char* n = SDL_GetScancodeName(static_cast<SDL_Scancode>(code));
    if (n && n[0]) return n;
    return "Key";
}

inline bool bindHeld(const Uint8* keys, int code) {
    if (code < 0) {
        return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(-code)) != 0;
    }
    if (!keys || code <= 0 || code >= SDL_NUM_SCANCODES) return false;
    return keys[code] != 0;
}

inline int encodeMouseBind(int sdlButton) { return -sdlButton; }

} // namespace sbs
