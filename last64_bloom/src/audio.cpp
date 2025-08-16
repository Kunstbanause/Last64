#include "audio.h"
#include <libdragon.h>
#include <stdlib.h>

SFXManager gSFXManager;

void SFXManager::init()
{
    wav64_open(&sfx_start,    "rom:/sfx/start.wav64");
    wav64_open(&sfx_join,     "rom:/sfx/join.wav64");
    wav64_open(&sfx_hits[0],  "rom:/sfx/hit1.wav64");
    wav64_open(&sfx_hits[1],  "rom:/sfx/hit2.wav64");
    wav64_open(&sfx_hits[2],  "rom:/sfx/hit3.wav64");
    wav64_open(&sfx_hits[3],  "rom:/sfx/hit4.wav64");
    next_hit_channel = 0;
}

void SFXManager::play(SfxId id)
{
    switch (id)
    {
        case SFX_LEVEL_UP:
            mixer_ch_play(0, &sfx_start.wave);
            break;
        case SFX_START:
            mixer_ch_play(0, &sfx_start.wave);
            break;
        case SFX_JOIN:
            mixer_ch_play(1, &sfx_join.wave);
            break;
        case SFX_HIT:
            mixer_ch_play(2 + next_hit_channel, &sfx_hits[rand() % 4].wave);
            next_hit_channel = (next_hit_channel + 1) % HIT_CHANNELS;
            break;
    }
}