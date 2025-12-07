#include "audio.h"
#include <libdragon.h>
#include <stdlib.h>

void SFXManager::init()
{
    wav64_open(&sfx_level_up, "rom:/sfx/level.wav64");
    wav64_open(&sfx_start,    "rom:/sfx/start.wav64");
    wav64_open(&sfx_join,     "rom:/sfx/join.wav64");
    wav64_open(&sfx_death,    "rom:/sfx/death.wav64");
    wav64_open(&sfx_music1,   "rom:/sfx/music2.wav64");
    // Enable looping for background music so it repeats when it reaches the end
    wav64_set_loop(&sfx_music1, true);
    wav64_open(&sfx_xp[0], "rom:/sfx/xp1.wav64");
    wav64_open(&sfx_xp[1], "rom:/sfx/xp2.wav64");
    wav64_open(&sfx_xp[2], "rom:/sfx/xp3.wav64");
    wav64_open(&sfx_xp[3], "rom:/sfx/xp4.wav64");
    wav64_open(&sfx_xp[4], "rom:/sfx/xp5.wav64");
    wav64_open(&sfx_xp[5], "rom:/sfx/xp6.wav64");
    wav64_open(&sfx_xp[6], "rom:/sfx/xp7.wav64");
    wav64_open(&sfx_xp[7], "rom:/sfx/xp8.wav64");
    wav64_open(&sfx_hits[0],  "rom:/sfx/hitA01.wav64");
    wav64_open(&sfx_hits[1],  "rom:/sfx/hitA02.wav64");
    wav64_open(&sfx_hits[2],  "rom:/sfx/hitA03.wav64");
    wav64_open(&sfx_hits[3],  "rom:/sfx/hitA04.wav64");
    wav64_open(&sfx_hits[4],  "rom:/sfx/hitA05.wav64");
    wav64_open(&sfx_hits[5],  "rom:/sfx/hitA06.wav64");
    wav64_open(&sfx_hits[6],  "rom:/sfx/hitA07.wav64");
    wav64_open(&sfx_hits[7],  "rom:/sfx/hitA08.wav64");
    wav64_open(&sfx_hits[8],  "rom:/sfx/hitA09.wav64");
    next_hit_channel = 0;
}

void SFXManager::play(SfxId id)
{
    switch (id)
    {
        case SFX_LEVEL_UP:
            mixer_ch_play(0, &sfx_level_up.wave);
            break;
        case SFX_START:
            mixer_ch_play(0, &sfx_start.wave);
            break;
        case SFX_JOIN:
            mixer_ch_play(1, &sfx_join.wave);
            break;
        case SFX_XP1:
            mixer_ch_play(1, &sfx_xp[0].wave);
            break;
        case SFX_XP2:
            mixer_ch_play(1, &sfx_xp[1].wave);
            break;
        case SFX_XP3:
            mixer_ch_play(1, &sfx_xp[2].wave);
            break;
        case SFX_XP4:
            mixer_ch_play(1, &sfx_xp[3].wave);
            break;
        case SFX_XP5:
            mixer_ch_play(1, &sfx_xp[4].wave);
            break;
        case SFX_XP6:
            mixer_ch_play(1, &sfx_xp[5].wave);
            break;
        case SFX_XP7:
            mixer_ch_play(1, &sfx_xp[6].wave);
            break;
        case SFX_XP8:
            mixer_ch_play(1, &sfx_xp[7].wave);
            break;
        case SFX_DEATH:
            mixer_ch_play(1, &sfx_death.wave);
            break;
        case SFX_MUSIC1:
            if (isMusicEnabled()) mixer_ch_play(2, &sfx_music1.wave);
            break;
        case SFX_HIT:
            mixer_ch_play(3 + next_hit_channel, &sfx_hits[rand() % sfx_hits_count].wave);
            next_hit_channel = (next_hit_channel + 1) % HIT_CHANNELS;
            break;
    }
}

void SFXManager::setVolume_Music(float volume, float fadeTime)
{
    // Clamp volume
    if(volume < 0.0f) volume = 0.0f;
    if(volume > 1.0f) volume = 1.0f;

    if (fadeTime <= 0.0f) {
        // Immediate set
        music_current_vol = volume;
        music_target_vol = volume;
        music_fade_duration = 0.0f;
        music_fade_elapsed = 0.0f;
        mixer_ch_set_vol(2, volume, volume);
        return;
    }

    music_target_vol = volume;
    music_fade_duration = fadeTime;
    music_fade_elapsed = 0.0f;
}

// Compatibility wrapper used by some older code compiled against the single-arg symbol
void SFXManager::setVolume_Music(float volume)
{
    setVolume_Music(volume, 0.0f);
}

void SFXManager::update(float delta)
{
    if (music_fade_duration > 0.0f && music_current_vol != music_target_vol) {
        music_fade_elapsed += delta;
        float t = music_fade_elapsed / music_fade_duration;
        if (t >= 1.0f) {
            music_current_vol = music_target_vol;
            music_fade_duration = 0.0f;
            music_fade_elapsed = 0.0f;
        } else {
            // Simple linear interpolation
            music_current_vol = music_current_vol + (music_target_vol - music_current_vol) * t;
        }
        mixer_ch_set_vol(2, music_current_vol, music_current_vol);
    }
}

void SFXManager::setMusicEnabled(bool enabled)
{
    if (!enabled) {
        // Stop music channel immediately
        mixer_ch_stop(2);
    } else {
        // Start music channel (looping is configured in init)
        mixer_ch_play(2, &sfx_music1.wave);
    }
    this->music_enabled = enabled;
}

bool SFXManager::isMusicEnabled() const
{
    return this->music_enabled;
}

void SFXManager::stop(SfxId id)
{
    switch (id)
    {
        case SFX_LEVEL_UP:
            mixer_ch_stop(0);
            break;
        case SFX_START:
            mixer_ch_stop(0);
            break;
        case SFX_JOIN:
            mixer_ch_stop(1);
            break;
        case SFX_DEATH:
            mixer_ch_stop(1);
            break;
        case SFX_MUSIC1:
            mixer_ch_stop(2);
            break;
        case SFX_HIT:
            // Stop all hit channels
            for (int i = 0; i < HIT_CHANNELS; i++) {
                mixer_ch_stop(3 + i);
            }
            break;
        case SFX_XP1:
        case SFX_XP2:
        case SFX_XP3:
        case SFX_XP4:
        case SFX_XP5:
        case SFX_XP6:
        case SFX_XP7:
        case SFX_XP8:
            mixer_ch_stop(1);
            break;
    }
}