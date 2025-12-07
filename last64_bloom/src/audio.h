#pragma once
#include <libdragon.h>

class SFXManager {
public:
    enum SfxId {
        SFX_LEVEL_UP,
        SFX_START,
        SFX_JOIN,
        SFX_DEATH,
        SFX_MUSIC1,
        SFX_HIT,
        SFX_XP1,
        SFX_XP2,
        SFX_XP3,
        SFX_XP4,
        SFX_XP5,
        SFX_XP6,
        SFX_XP7,
        SFX_XP8
    };

    void init();
    void play(SfxId id);
    void stop(SfxId id);
    // Set music volume immediately or over a fade time (seconds)
    void setVolume_Music(float volume, float fadeTime = 0.0f);
    // Compatibility wrapper: single-arg version (keeps previous symbol)
    void setVolume_Music(float volume);
    // Called each frame to advance fades (delta in seconds)
    void update(float delta);
    // Enable/disable music globally; when disabled music play calls are ignored
    void setMusicEnabled(bool enabled);
    bool isMusicEnabled() const;

private:
    static const int HIT_CHANNELS = 11; // We have 16 channels #0-15
    wav64_t sfx_level_up;
    wav64_t sfx_start;
    wav64_t sfx_join;
    wav64_t sfx_death;
    wav64_t sfx_music1;
    wav64_t sfx_xp[8];  // Array of 8 xp sounds
    wav64_t sfx_hits[9];
    static const int sfx_hits_count = sizeof(sfx_hits) / sizeof(sfx_hits[0]);
    int next_hit_channel = 0;
    // Fading state for music volume
    float music_current_vol = 1.0f; // current linear volume (0.0 - 1.0)
    float music_target_vol = 1.0f;
    float music_fade_duration = 0.0f;
    float music_fade_elapsed = 0.0f;
    bool music_enabled = true;
};

extern SFXManager gSFXManager;