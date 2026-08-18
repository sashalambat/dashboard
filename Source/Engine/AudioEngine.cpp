#include "Engine/AudioEngine.h"

#include "Engine/SBSLog.h"

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#include <SDL_mixer.h>
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace sbs {
namespace {

struct HeldChunk {
    std::vector<int16_t> pcm;
    Mix_Chunk chunk{};
};

HeldChunk makeHeld(float freq, int ms, float volume, bool noise) {
    const int rate = 22050;
    const int samples = std::max(64, rate * ms / 1000);
    HeldChunk h;
    h.pcm.resize(static_cast<size_t>(samples));
    Rng rng(static_cast<uint32_t>(freq * 10));
    for (int i = 0; i < samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(rate);
        const float env = 1.0f - static_cast<float>(i) / static_cast<float>(samples);
        float s = std::sin(2.0f * 3.14159265f * freq * t);
        if (noise) s = s * 0.35f + (rng.nextFloat() * 2.0f - 1.0f);
        s *= env * volume;
        h.pcm[static_cast<size_t>(i)] = static_cast<int16_t>(clampf(s, -1.0f, 1.0f) * 18000.0f);
    }
    h.chunk.allocated = 0;
    h.chunk.alen = static_cast<Uint32>(h.pcm.size() * sizeof(int16_t));
    h.chunk.abuf = reinterpret_cast<Uint8*>(h.pcm.data());
    h.chunk.volume = MIX_MAX_VOLUME;
    return h;
}

std::map<std::string, HeldChunk> gBank;

} // namespace

bool AudioEngine::init(const AudioSettings& settings) {
    settings_ = settings;
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            logWarn("Audio subsystem unavailable");
            return false;
        }
    }
    if (Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 512) != 0) {
        logWarn(std::string("SDL_mixer open failed: ") + Mix_GetError());
        return false;
    }
    Mix_AllocateChannels(24);
    generateBank();
    ready_ = true;
    return true;
}

void AudioEngine::shutdown() {
    if (ready_) {
        Mix_CloseAudio();
        ready_ = false;
    }
    gBank.clear();
}

void AudioEngine::setSettings(const AudioSettings& settings) {
    settings_ = settings;
}

void AudioEngine::generateBank() {
    gBank["ar"] = makeHeld(420, 70, 0.7f, true);
    gBank["shot"] = makeHeld(180, 140, 0.8f, true);
    gBank["mg"] = makeHeld(360, 50, 0.65f, true);
    gBank["sniper"] = makeHeld(140, 220, 0.9f, true);
    gBank["rail"] = makeHeld(880, 180, 0.8f, false);
    gBank["rocket"] = makeHeld(90, 260, 0.9f, true);
    gBank["plasma"] = makeHeld(620, 80, 0.6f, false);
    gBank["pulse"] = makeHeld(240, 160, 0.75f, true);
    gBank["nade"] = makeHeld(70, 300, 0.95f, true);
    gBank["ui"] = makeHeld(660, 60, 0.4f, false);
    gBank["voiceA"] = makeHeld(440, 180, 0.5f, false);
    gBank["voiceB"] = makeHeld(520, 180, 0.5f, false);
    gBank["start"] = makeHeld(300, 400, 0.6f, false);
    gBank["end"] = makeHeld(180, 500, 0.6f, false);
    for (auto& kv : gBank) {
        kv.second.chunk.abuf = reinterpret_cast<Uint8*>(kv.second.pcm.data());
        kv.second.chunk.alen = static_cast<Uint32>(kv.second.pcm.size() * sizeof(int16_t));
    }
}

void AudioEngine::queueTone(float freq, float ms, float volume, float pan) {
    (void)freq;
    (void)ms;
    playSfx("ui", volume, pan);
}

void AudioEngine::playSfx(const char* name, float volume, float pan) {
    if (!ready_ || !name) return;
    auto it = gBank.find(name);
    if (it == gBank.end()) return;
    const int ch = Mix_PlayChannel(-1, &it->second.chunk, 0);
    if (ch < 0) return;
    const int vol = static_cast<int>(MIX_MAX_VOLUME * settings_.master * settings_.sfx * volume);
    Mix_Volume(ch, vol);
    const int left = static_cast<int>(clampf(128.0f - pan * 127.0f, 0.0f, 255.0f));
    Mix_SetPanning(ch, static_cast<Uint8>(left), static_cast<Uint8>(255 - left));
}

void AudioEngine::playWeapon(WeaponId id, Faction faction, float distance) {
    (void)faction;
    float vol = clampf(1.0f - distance / 40.0f, 0.08f, 1.0f);
    const char* n = "ar";
    switch (id) {
        case WeaponId::SBSCombatShotgun: n = "shot"; break;
        case WeaponId::SBSMachineGun: n = "mg"; break;
        case WeaponId::SBSSniperRifle: n = "sniper"; break;
        case WeaponId::Railgun: n = "rail"; break;
        case WeaponId::RocketLauncher: n = "rocket"; break;
        case WeaponId::PlasmaRifle: n = "plasma"; break;
        case WeaponId::PulseCannon: n = "pulse"; break;
        case WeaponId::Grenade: n = "nade"; break;
        default: n = "ar"; break;
    }
    playSfx(n, vol, 0.0f);
}

void AudioEngine::playVoice(Faction faction, const char* cue) {
    (void)cue;
    playSfx(faction == Faction::CyberDominion ? "voiceB" : "voiceA", settings_.voice, 0.0f);
}

void AudioEngine::playUi(const char* cue) {
    (void)cue;
    playSfx("ui", 0.5f, 0.0f);
}

void AudioEngine::playAnnouncement(const char* text) {
    if (!text) return;
    const std::string s(text);
    if (s.find("start") != std::string::npos || s.find("Match start") != std::string::npos) playSfx("start", 0.8f, 0);
    else if (s.find("victory") != std::string::npos || s.find("complete") != std::string::npos) playSfx("end", 0.8f, 0);
    else playSfx("voiceA", 0.6f, 0);
}

void AudioEngine::updateListener(const Vec2& pos, float yaw) {
    listener_ = pos;
    listenerYaw_ = yaw;
}

} // namespace sbs
