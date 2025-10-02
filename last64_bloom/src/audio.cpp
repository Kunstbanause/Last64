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
        case SFX_DEATH:
            mixer_ch_play(1, &sfx_death.wave);
            break;
        case SFX_MUSIC1:
            mixer_ch_play(2, &sfx_music1.wave);
            break;
        case SFX_HIT:
            mixer_ch_play(3 + next_hit_channel, &sfx_hits[rand() % sfx_hits_count].wave);
            next_hit_channel = (next_hit_channel + 1) % HIT_CHANNELS;
            break;
    }
}

void SFXManager::setVolume_Music(float volume)
{
   mixer_ch_set_vol(2, volume, volume);
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
    }
}