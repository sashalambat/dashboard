#pragma once

#include "Engine/SBSTypes.h"
#include "Engine/SBSMath.h"
#include "Game/SaveSystem.h"

namespace sbs {

class AudioEngine {
public:
    bool init(const AudioSettings& settings);
    void shutdown();
    void setSettings(const AudioSettings& settings);
    void playSfx(const char* name, float volume, float pan);
    void playWeapon(WeaponId id, Faction faction, float distance);
    void playVoice(Faction faction, const char* cue);
    void playUi(const char* cue);
    void playAnnouncement(const char* text);
    void updateListener(const Vec2& pos, float yaw);

private:
    void generateBank();
    void queueTone(float freq, float ms, float volume, float pan);

    AudioSettings settings_{};
    bool ready_ = false;
    Vec2 listener_{};
    float listenerYaw_ = 0.0f;
};

} // namespace sbs
